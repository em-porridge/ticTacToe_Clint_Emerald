//
// Created by emu on 2021-04-07.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include "fsm.h"
#include "v3_TTT_FSM.h"
#include "GameCollection_LinkedList.h"
#include "shared.h"

int prepare_tcp_listening_socket(struct sockaddr_in *addr);
int prepare_upd_socket(struct sockaddr_in *addr);
static int handle_new_connection(node *lobby, int cfd, uint32_t uid, uint8_t game_type);


static int handle_client_disconnect(int cfd);
static int read_new_client_request(int cfd);
static int send_error_code(int cfd, int error_code);
static int send_new_game_code(int cfd, uint32_t uid);
static int send_start_ttt_game_code(int cfd_x, int cfd_o);
static int out_of_turn_client_request(int cfd);



int main() {
    fd_set rfds, ready_sockets;
    struct sockaddr_in addr, cliaddr;
    int retval, game_id;
    char buffer[1024];
    uint32_t unique_game_id;

    int sfd = prepare_tcp_listening_socket(&addr);
    int updfd = prepare_upd_socket(&addr);

    // initialize a TTT lobby and a RPS lobby

    node *game_lobby;
    init_lobbies(&game_lobby);
    printf("Initialized Lobbies");

    unique_game_id = 3;

    FD_ZERO(&rfds);
    FD_SET((u_int) sfd, &rfds);
    FD_SET((u_int) updfd, &rfds);

    while (1) {
        ssize_t num_read;
        int cfd;
        ready_sockets = rfds;

        retval = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        if (retval == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {

            if (FD_ISSET((u_int) i, &ready_sockets)) {
                if (i == sfd) {
                    printf("\n=========== CONNECTION ==========\n");
                    cfd = accept(sfd, NULL, NULL);
                    unique_game_id++;
                    FD_SET((u_int) cfd, &rfds);
                } else {
                    cfd = i;
                    // todo error check
                    int32_t client_game_uid;
                    num_read = read(cfd, &client_game_uid, sizeof (client_game_uid));

                    if (num_read < 1) {
                        printf("++++++ Disconnect +++++ \n");
                        node * disconnected_node = return_link_by_uid(&game_lobby, client_game_uid);
                        // todo update with this RPS
                        if(disconnected_node->TTTGame.client_o == cfd) {
                            handle_client_disconnect(cfd);
                        } else if (disconnected_node->TTTGame.client_x == cfd) {
                            handle_client_disconnect(cfd);
                        } else {
                            // wah
                        }
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    } else {
                        if(client_game_uid == 0) {
                            int ttt_rps = read_new_client_request(cfd);
                            handle_new_connection(game_lobby, cfd, unique_game_id, ttt_rps);
                        } else {
                            node * game_node = return_link_by_uid(&game_lobby, client_game_uid);
                            // todo need a better way of checking :/
                            if(game_node->TTTGame.client_x) {
                                if (game_node->TTTGame.fd_current_player == cfd) {
                                    mainaroo(&game_node->TTTGame);
                                } else {
                                    out_of_turn_client_request(cfd);
                                }
                                if(game_node->TTTGame.game_over) {
                                    // disconnect??
                                }
                            } else {
                                // run RPS
                            }
                        }
                    }
                }
            }
        }
    }
}


/**
 * Creates a TCP server socket.
 *
 * @return int (socket) file descriptor
 */
int prepare_tcp_listening_socket(struct sockaddr_in *addr) {
//    struct sockaddr_in addr;

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(addr, 0, sizeof(struct sockaddr_in));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(PORT);
    addr->sin_addr.s_addr = htonl(INADDR_ANY);

    printf("PORT: %d \n", addr->sin_port);

    if (bind(sfd, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Bind failed!");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, BACKLOG) < 0) {
        perror("Listen failed!");
        exit(EXIT_FAILURE);
    }
    printf("SFD FD: %d \n", sfd);
    return sfd;
}

/**
 * Creates a TCP server socket.
 *
 * @return int (socket) file descriptor
 */
int prepare_upd_socket(struct sockaddr_in *addr){
    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(udpfd, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
    printf("UDP FD: %d \n", udpfd);

    return udpfd;
}


/**
 * Reads client request according to protocol
 *
 * @return the game type (1, 2) the client wants to play
 * **/
static int read_new_client_request(int cfd){
    // protocol
    // todo add error checking for ALLLLL these values
    int8_t req_type;
    int8_t req_context;
    int8_t payloadLength = 0;

    int8_t version;
    int8_t game_id;

    read(cfd, &req_type, sizeof (req_type));
    read(cfd, &req_context, sizeof (req_context));
    // error
    read(cfd, &payloadLength, sizeof (payloadLength));

    // todo if statement here for RPS ?
    read(cfd, &version, sizeof (version));
    read(cfd, &game_id, sizeof (game_id));

    printf("--- NEW CLIENT INFO --- \n  req_context := %u \n payloadLength:= %u \n gameID:= %u \n ", req_context, payloadLength, game_id);
    return game_id;
}

/**
 * Receives a new connection, adds client to lobby and starts a new game if the lobby is full
 *
 *@return success/error
 * **/
static int handle_new_connection(node *lobby, int cfd, uint32_t uid, uint8_t game_type) {
    if (game_type == 1) {
        if (lobby->TTTGame.client_x > 0) {

            send_new_game_code(cfd, uid);

            SingleTTTGameEnv newGameEnv;
            newGameEnv.client_x = lobby->TTTGame.client_x;
            newGameEnv.client_o = cfd;
            newGameEnv.fd_current_player = newGameEnv.client_x;
            newGameEnv.unique_game_id = lobby->TTTGame.unique_game_id;

            insert_new_ttt_game(&lobby, &newGameEnv);
            print_ttt_collection(&lobby);

            send_start_ttt_game_code(newGameEnv.client_x, newGameEnv.client_o);
            reset_ttt_lobby(&lobby);

            return 0;
        } else if(lobby->TTTGame.client_x == 0) {
            lobby->TTTGame.client_x = cfd;
            lobby->TTTGame.unique_game_id = uid;
            send_new_game_code(cfd, uid);
            return 0;
        }
    } else if (game_type == 2){
        // its a RPS game
    } else {
        // error
    }
    return -1;
}

static int handle_client_disconnect(int cfd) {
    int8_t status = UPDATE;
    int8_t context = UPDATE_DISCONNECT;
    int8_t payload_length = 0;

    send(cfd, &status, sizeof (status), 0);
    send(cfd, &context, sizeof (context), 0);
    send(cfd, &payload_length, sizeof (payload_length), 0);

    return 0;
}



// do we need this?
static int out_of_turn_client_request(int cfd) {
    int16_t irrelevant_data;
    int8_t payload_length;
    int8_t payload;

    read(cfd, &irrelevant_data, sizeof (irrelevant_data));
    read(cfd, &payload_length, sizeof (payload_length));

    for(int i = 0; i < payload_length;  i++) {
        read(cfd, &payload, sizeof (payload));
    }
    send_error_code(cfd, GAME_ERROR_ACTION_OUT_OF_TURN);
    return 0;
}


static int send_error_code(int cfd, int error_code) {
    int8_t status = error_code;
    int8_t context = INFORMATION;
    int8_t payload_length = 0;

    send(cfd, &status, sizeof(status), 0);
    send(cfd, &context, sizeof(context), 0);
    send(cfd, &payload_length, sizeof(payload_length), 0);
}


static int send_new_game_code(int cfd, uint32_t uid) {
    int8_t status = SUCCESS;
    int8_t context = CONFIRMATION;
    int8_t payload_length = 4;
    int32_t payload = uid;

    send(cfd, &status, sizeof(status), 0);
    send(cfd, &context, sizeof(context), 0);
    send(cfd, &payload_length, sizeof(payload_length), 0);
    send(cfd, &payload, sizeof(payload), 0);
}

static int send_start_ttt_game_code(int cfd_x, int cfd_o) {
    int8_t status = UPDATE_START_GAME;
    int8_t context = CONFIRMATION;
    int8_t payload_length = 0;
    int8_t payload_x = X;
    int8_t payload_o = O;

    send(cfd_x, &status, sizeof(status), 0);
    send(cfd_x, &context, sizeof(context), 0);
    send(cfd_x, &payload_length, sizeof(payload_length), 0);
    send(cfd_x, &payload_x, sizeof(payload_x), 0);

    send(cfd_o, &status, sizeof(status), 0);
    send(cfd_o, &context, sizeof(context), 0);
    send(cfd_o, &payload_length, sizeof(payload_length), 0);
    send(cfd_o, &payload_o, sizeof(payload_x), 0);
}

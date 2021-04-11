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

static int read_data(int cfd, data *client_data);

static int handle_client_disconnect(int cfd);
static int send_error_code(int cfd, int error_code);
static int send_new_game_code(int cfd, uint32_t uid);
static int send_start_ttt_game_code(int cfd_x, int cfd_o);

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
    unique_game_id = 3;

    data *received_data;

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
                    num_read = read_data(cfd, received_data);
                    if (num_read < 1) {
                        printf("++++++ Disconnect +++++ \n");
                        node * disconnected_node = return_link_by_uid(&game_lobby, received_data->uid);

                        if(disconnected_node->TTTGame.client_o == cfd) {
                            handle_client_disconnect(cfd);

                        } else if (disconnected_node->TTTGame.client_x == cfd) {
                            handle_client_disconnect(cfd);

                        } else {
                            // todo update with RPS
                        }
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    } else {
                        if(received_data->uid == 0) {
                            printf("newGame");
                            handle_new_connection(game_lobby, cfd, unique_game_id, received_data->payload_second_byte);
                        } else {
                            node * game_node = return_link_by_uid(&game_lobby, received_data->uid);

                            // todo need a better way of checking :/ !! ADD a game type to main node area
                            if(game_node->TTTGame.client_x) {
                                if (game_node->TTTGame.fd_current_player == cfd) {
                                    mainaroo(&game_node->TTTGame);
                                } else {
                                    send_error_code(cfd, GAME_ERROR_ACTION_OUT_OF_TURN);
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


static int read_data(int cfd, data *client_data){
    char byteArray[256];

    if(recv(cfd, &byteArray, sizeof (byteArray), 0) == 0) {
        return -1;
    };

    // read array
    client_data->uid = byteArray[0];
    client_data->uid |= byteArray[1] >> 8;
    client_data->uid |= byteArray[2] >> 16;
    client_data->uid |= byteArray[3] >> 24;

    client_data->req_type = byteArray[4];
    client_data->context = byteArray[5];
    client_data->payload_length = byteArray[6];

    if(client_data->payload_length == 1) {
        client_data->payload_first_byte = byteArray[7];
        client_data->payload_second_byte = '\0';
        client_data->payload_third_byte='\0';
        // todo error check -- payload error if not within range
    } else if (client_data->payload_length == 2) {
        // todo error check -- payload error if not within expected number range
        client_data->payload_first_byte = byteArray[7];
        client_data->payload_second_byte = byteArray[8];
        client_data->payload_third_byte = '\0';
    } else {
        // payload is zero and we expect nothin more
    }
    return 1;
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

            //todo send as array
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

static int send_error_code(int cfd, int error_code) {
    int8_t status = error_code;
    int8_t context = INFORMATION;
    int8_t payload_length = 0;

    unsigned char byte_array[3];
    byte_array[0] = status;
    byte_array[1] = context >> 8;
    byte_array[2] = payload_length >> 16;

    send(cfd, &byte_array, sizeof (byte_array), 0);

    return 0;
}

static int send_new_game_code(int cfd, uint32_t uid) {
    int8_t status = SUCCESS;
    int8_t context = CONFIRMATION;
    int8_t payload_length = 4;
    int32_t payload = uid;

    unsigned char byte_array[7];

    byte_array[0] = status;
    byte_array[1] = context;
    byte_array[2] = payload_length;
    byte_array[3] = (payload >> 0);
    byte_array[4] = (payload >> 8);
    byte_array[5] = (payload >> 16);
    byte_array[6] = (payload >> 24);

    send(cfd, &byte_array, sizeof (byte_array), 0);

    return 0;
}

static int send_start_ttt_game_code(int cfd_x, int cfd_o) {
    int8_t status = UPDATE_START_GAME;
    int8_t context = CONFIRMATION;
    int8_t payload_length = 0;
    int8_t payload_x = X;
    int8_t payload_o = O;

    unsigned char byte_array_x[4];
    unsigned char byte_array_o[4];

    byte_array_x[0] = status;
    byte_array_x[1] = context;
    byte_array_x[2] = payload_length;
    byte_array_x[3] = payload_x;

    byte_array_o[0] = status;
    byte_array_o[1] = context;
    byte_array_o[2] = payload_length;
    byte_array_o[3] = payload_o;

    send(cfd_o, &byte_array_o, sizeof (byte_array_o), 0);
    send(cfd_x, &byte_array_x, sizeof (byte_array_x), 0);
}


static int reset_data(data * old_data) {

    old_data->req_type = 0;
    old_data->context = 0;
    old_data->uid = 0;
    old_data->payload_length = 0;

    return 1;
}
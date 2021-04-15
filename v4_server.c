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
#include "RPS_FSM.h"
#include "GameCollection_LinkedList.h"
#include "shared.h"

int prepare_tcp_listening_socket(struct sockaddr_in *addr);
int prepare_upd_socket(struct sockaddr_in *addr);
static int handle_new_connection(node *lobby, int cfd, uint32_t uid, uint8_t game_type);

static int read_data(int cfd, data *client_data);

static int client_disconnect(node *lobby, int cfd);
static int handle_client_disconnect(int cfd);
static int send_error_code(int cfd, int error_code);
static int send_new_game_code(int cfd, uint32_t uid);
static int send_start_ttt_game_code(int cfd_x, int cfd_o);
static int send_start_rps_game_code(int cfd_one, int cfd_two);

int main() {
    fd_set rfds, ready_sockets;
    struct sockaddr_in addr, cliaddr;
    int retval, game_id;
    unsigned char buffer[1024];
    uint32_t unique_game_id;

    int sfd = prepare_tcp_listening_socket(&addr);
    int udpfd = prepare_upd_socket(&addr);

    // initialize a TTT lobby and a RPS lobby
    node *game_lobby;
    init_lobbies(&game_lobby);
    unique_game_id = 3;

    data *received_data = malloc(sizeof *received_data);

    FD_ZERO(&rfds);
    FD_SET((u_int) sfd, &rfds);
    FD_SET((u_int) udpfd, &rfds);

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
                        client_disconnect(game_lobby, cfd);
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);

                    } else {
                        if(received_data->uid == 0) {
                            handle_new_connection(game_lobby, cfd, unique_game_id, received_data->payload_second_byte);
                        } else {
                            node * game_node = return_link_by_uid(&game_lobby, received_data->uid);
                            if(game_node == NULL) {
                                // send uid error
                            } else {
                                // check the game type
                                if(game_node->game_type == TTT_GAME) {
                                    if (game_node->TTTGame.fd_current_player == cfd) {
                                        game_node->TTTGame.FSM_data_reads = *received_data;
                                        game_node->TTTGame.received_move = received_data->payload_first_byte;
                                        mainaroo(&game_node->TTTGame);
                                        // reset data here
                                    } else {
                                        send_error_code(cfd, GAME_ERROR_ACTION_OUT_OF_TURN);
                                    }
                                } else {
                                    game_node->RPSGame.fd_current_client = cfd;

                                    game_node->RPSGame.move_received = received_data->payload_first_byte;
                                    mainzees(&game_node->RPSGame);
                                    // send to FSM

                                }
                            }


                        }
                    }
                }
            } else if (FD_ISSET(udpfd, &ready_sockets)){

                socklen_t len = sizeof (cliaddr);
                bzero(buffer, sizeof (buffer));
                ssize_t n;

                n= recvfrom(udpfd, buffer, sizeof (buffer), 0, (struct sockaddr*)&cliaddr, &len);
                printf("UID from UDP %u \n", buffer[0]);

//                node *check_pair = return_link_by_uid(&game_lobby, buffer[0]);
//                sendto(udpfd, buffer, sizeof (buffer), 0, (struct sockaddr*)&cliaddr, len);
//                printf("SIZE OF AUDIO : %d\n", buffer);

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
    uint8_t byteArray[256];

    if(recv(cfd, &byteArray, sizeof (byteArray), 0) == 0) {
        return -1;
    };

    client_data->uid = htonl(*(uint32_t*)byteArray);
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
        // if there is already a client in the lobby
        if (lobby->TTTGame.client_x > 0) {
            // assign new uid
            send_new_game_code(cfd, uid);
            SingleTTTGameEnv newGameEnv;
            newGameEnv.client_x = lobby->TTTGame.client_x;
            newGameEnv.client_o = cfd;
            newGameEnv.fd_current_player = newGameEnv.client_x;
            newGameEnv.game_over = false;
            // set uids
            newGameEnv.client_x_uid = lobby->TTTGame.client_x_uid;
            newGameEnv.client_o_uid = uid;
            newGameEnv.play_count = 0;
            for(int i = 0; i < 9; i++) {
                newGameEnv.game_board[i] = BLANK_SPACE;
            }
            insert_new_ttt_game(&lobby, &newGameEnv);
            print_ttt_collection(&lobby);
            send_start_ttt_game_code(newGameEnv.client_x, newGameEnv.client_o);
            reset_ttt_lobby(&lobby);
            return 0;
        } else if(lobby->TTTGame.client_x == 0) {
            lobby->TTTGame.client_x = cfd;
            lobby->TTTGame.client_x_uid = uid;

            send_new_game_code(cfd, uid);
            return 0;
        }
    } else if (game_type == 2){
        if(lobby->RPSGame.fd_client_player_one > 0) {
            send_new_game_code(cfd, uid);
            SingleRPSGameEnv newGameEnv;
            newGameEnv.fd_client_player_one = lobby->RPSGame.fd_client_player_one;
            newGameEnv.fd_client_player_two = cfd;

            newGameEnv.unique_game_id_player_one = lobby->RPSGame.unique_game_id_player_one;
            newGameEnv.unique_game_id_player_two = uid;

            insert_new_rps_game(&lobby, &newGameEnv);
            printf("Sending Invites...\n");
            send_start_rps_game_code(newGameEnv.fd_client_player_one, newGameEnv.fd_client_player_two);
            reset_rps_lobby(&lobby);
            printf("SENT! \n");

            return 0;
        } else if(lobby->RPSGame.fd_client_player_one == 0) {
            lobby->RPSGame.fd_client_player_one = cfd;
            lobby->RPSGame.unique_game_id_player_one = uid;

            send_new_game_code(cfd, uid);
            return 0;
        } else {
            // dunno
        }
    } else {
        // error
    }
    return -1;
}


static int handle_client_quiting(int cfd) {


}

static int handle_client_disconnect(int cfd) {

    int8_t status = UPDATE;
    int8_t context = UPDATE_DISCONNECT;
    int8_t payload_length = 0;

    unsigned char byte_array[3];

    byte_array[0] = status;
    byte_array[1] = context;
    byte_array[2] = payload_length;

    send(cfd, &byte_array, sizeof (byte_array), 0);
    close(cfd);
    return 0;
}

static int send_error_code(int cfd, int error_code) {
    int8_t status = error_code;
    int8_t context = INFORMATION;
    int8_t payload_length = 0;

    unsigned char byte_array[3];
    byte_array[0] = status;
    byte_array[1] = context;
    byte_array[2] = payload_length;

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

    byte_array[3] = (payload >> 24);
    byte_array[4] = (payload >> 16);
    byte_array[5] = (payload >> 8);
    byte_array[6] = (payload);

    send(cfd, &byte_array, sizeof (byte_array), 0);

    return 0;
}


static int send_start_rps_game_code(int cfd_one, int cfd_two) {

    if(cfd_two == cfd_one) {
        return -1;
    }

    uint8_t status = UPDATE;
    uint8_t context = UPDATE_START_GAME;
    uint8_t payload_length = 0;

    unsigned char byte_array_play[4];

    byte_array_play[0] = status;
    byte_array_play[1] = context;
    byte_array_play[2] = payload_length;

    send(cfd_one, &byte_array_play, sizeof (byte_array_play), 0);
    send(cfd_two, &byte_array_play, sizeof (byte_array_play), 0);
    return 0;
}

static int send_start_ttt_game_code(int cfd_x, int cfd_o) {

    uint8_t status = UPDATE;
    uint8_t context = UPDATE_START_GAME;
    uint8_t payload_length = 1;
    uint8_t payload_x = X;
    uint8_t payload_o = O;

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

static int client_disconnect(node *lobby, int cfd){
    // check lobbies
    if(lobby->RPSGame.fd_client_player_one == cfd) {
        reset_rps_lobby(&lobby);
    } if (lobby->TTTGame.client_x == cfd) {
        reset_ttt_lobby(&lobby);
    } else {
        node * disconnect = return_link_by_cfd(&lobby, cfd);
        if (disconnect == NULL ) {
            // do nothing??
        } else {
            if(disconnect->game_type == 1 ) {
                if(disconnect->TTTGame.client_x == cfd) {
                    handle_client_disconnect(disconnect->TTTGame.client_o);
                    // delete node
                } else {
                    handle_client_disconnect(disconnect->TTTGame.client_x);
                }
            } else if (disconnect->game_type == 2) {
                if(disconnect->RPSGame.fd_client_player_one == cfd) {
                    handle_client_disconnect(disconnect->RPSGame.fd_client_player_two);
                } else {
                    handle_client_disconnect(disconnect->RPSGame.fd_client_player_one);
                }
            }
        }
    }
}

static int reset_data(data * old_data) {

    old_data->req_type = 0;
    old_data->context = 0;
    old_data->uid = 0;
    old_data->payload_length = 0;

    return 1;
}
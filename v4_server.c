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

static int read_new_client_request(int cfd);
static int handle_new_connection(node *lobby, int cfd, uint32_t uid, uint8_t game_type);

//static int handle_existing_client_request(int cfd);

//static void create_new_game(GameLobby *lobby, GameEnvironment *new_env);
//static void create_from_existing_game(GameLobby *lobby, GameEnvironment *existing_env);
//static int find_disconnected(int cfd, SingleGame * collection);
//
//static void handle_incoming_cfd(node *lobby, int cfd, int game_id);
//static void send_new_game_data(GameEnvironment *fromNode, int new_game_ID);
//static void handle_disconnect(node *lobby, int cfd);

static void handle_incoming_ttt_connection(node *lobby, int cfd, uint32_t uid);

static void read_uid(int cfd);
static void read_client_request(int cfd);


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
                    int32_t game;

                    // read unique id
                    num_read = read(cfd, &game, sizeof (game));

                    // todo check if there is uid?? -- results in error if coming from new connection

                    if (num_read < 1) {
                        printf("++++++ Disconnect +++++ \n");
//                        handle_disconnect(Lobby, cfd);
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    } else {
                        // if game uid is zero, add client to appropriate game lobby
                        if(game == 0) {
                            int ttt_rps = read_new_client_request(cfd);
                            handle_new_connection(game_lobby, cfd, unique_game_id, ttt_rps);
                        } else {
                            node * game_node = return_link_by_uid(&game_lobby, game);

                            // todo need a better way of checking :/
                            if(game_node->TTTGame.client_x) {
                                mainaroo(&game_node->TTTGame);
                                if(game_node->TTTGame.game_over) {
                                    // disconnect??
                                }
                            } else {
                                // run RPS
                            }
                        }
                        // FSM turn - if game over - disconnect all?? -- todo figure out what ot do on diconnect
//                        if(execute_turn(&Lobby, game, position_request)==1){
//                            // new game
//                            printf(" **** Rematch **** \n");
//                            GameEnvironment * rematching_game;
//                            rematching_game = rematch(&Lobby, game);
//                            send_new_game_data(rematching_game, game);
//                        }
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
    int8_t version;
    int8_t game_id;
    int8_t req_type;
    int8_t req_context;
    int8_t payloadLength = 0;
    int8_t catchPayload;

    read(cfd, &req_type, sizeof (req_type));
    read(cfd, &req_context, sizeof (req_context));
    read(cfd, &payloadLength, sizeof (payloadLength));

    // todo if statement here for RPS ?
    read(cfd, &version, sizeof (version));
    read(cfd, &game_id, sizeof (game_id));

    printf("--- NEW CLIENT INFO --- \n req_type := %u \n  req_context := %u \n payloadLength:= %u \n gameID:= %u \n ", req_type, req_context, payloadLength, game_id);
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
            send(cfd, &uid, sizeof (uint32_t), 0);

            SingleTTTGameEnv newGameEnv;
            newGameEnv.client_x = lobby->TTTGame.client_x;
            newGameEnv.client_o = cfd;
            newGameEnv.unique_game_id = lobby->TTTGame.unique_game_id;

            insert_new_ttt_game(&lobby, &newGameEnv);
            print_ttt_collection(&lobby);
            // todo start a new game
            reset_ttt_lobby(&lobby);

            return 0;
        } else if(lobby->TTTGame.client_x == 0) {
            lobby->TTTGame.client_x = cfd;
            lobby->TTTGame.unique_game_id = uid;
            send(cfd, &uid, sizeof (uint32_t), 0);
            return 0;
        }
    } else if (game_type == 2){
        // its a RPS game
    } else {
        // error
    }
    return -1;
}



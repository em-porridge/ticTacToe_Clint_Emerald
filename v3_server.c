
#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include "fsm.h"
#include "game_collection_linked_list.h"


// todo make a .h file
#include "v2_FSM.h"

#include "shared.h"

int prepare_server();

static void create_new_game(GameLobby *lobby, GameEnvironment *new_env);
static void create_from_existing_game(GameLobby *lobby, GameEnvironment *existing_env);
static int find_disconnected(int cfd, SingleGame * collection);

static void handle_incoming_cfd(node *lobby, int cfd, int game_id);
static void send_new_game_data(GameEnvironment *fromNode, int new_game_ID);
static void handle_disconnect(node *lobby, int cfd);

int main() {
    fd_set rfds, ready_sockets;
    int retval, game_id;
    int sfd = prepare_server();

    node * Lobby;
    init(&Lobby);
    game_id = 3;

    FD_ZERO(&rfds);
    FD_SET((u_int) sfd, &rfds);

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
                    printf("=========== CONNECTION ==========\n");
                    cfd = accept(sfd, NULL, NULL);
                    handle_incoming_cfd(Lobby, cfd, game_id);
                    game_id++;
                    FD_SET((u_int) cfd, &rfds);
                } else {
                    cfd = i;

                    // reads protocol
                    int8_t game;
                    int8_t position_request;
                    num_read = read(cfd, &game, sizeof (game));

                    if (num_read < 1) {
                        // handle diconnect
                        printf("++++++ Disconnect +++++");
                        handle_disconnect(Lobby, cfd);
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    } else {
                        read(cfd, &position_request, 1);
                        // FSM turn - if game over - rematch
                        if(execute_turn(&Lobby, game, position_request)==1){
                            // new game
                            printf(" ** Rematch ** h\n");
                            GameEnvironment * rematching_game;
                            rematching_game = rematch(&Lobby, game);
                            send_new_game_data(rematching_game, game);
                        }
                    }
                }
            }
        }
    }
}


/**
 * Creates a server socket.
 *
 * @return int (socket) file descriptor
 */
int prepare_server() {
    struct sockaddr_in addr;

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("PORT: %d \n", addr.sin_port);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Bind failed!");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, BACKLOG) < 0) {
        perror("Listen failed!");
        exit(EXIT_FAILURE);
    }

    return sfd;
}

// handles new connections. creates new game if lobby full
static void handle_incoming_cfd(node *lobby, int cfd, int game_id){
    if(lobby->game_env.fd_client_X == 0) {
        lobby->game_env.fd_client_X = cfd;
    } else if (lobby->game_env.fd_client_X > 0 && lobby->game_env.fd_client_O == 0) {
        lobby->game_env.fd_client_O = cfd;
        printf("Pair Found :) \n");

        insert_new_game(&lobby, game_id, lobby->game_env.fd_client_X, lobby->game_env.fd_client_O);
        send_new_game_data(&lobby->game_env, game_id);
        print(&lobby);
        reset_lobby(&lobby);
    }
}

// sends new game data
static void send_new_game_data(GameEnvironment *fromNode, int new_game_ID) {
    uint8_t welcome = WELCOME;
    uint8_t assign_x = X;
    uint8_t assign_o = O;

    send(fromNode->fd_client_X, &welcome, sizeof (welcome), 0);
    send(fromNode->fd_client_X, &assign_x, sizeof (assign_x), 0);
    send(fromNode->fd_client_X, &new_game_ID, sizeof (fromNode->game_ID), 0);
    send(fromNode->fd_client_O, &welcome, sizeof (welcome), 0);
    send(fromNode->fd_client_O, &assign_o, sizeof (assign_o), 0);
    send(fromNode->fd_client_O, &new_game_ID, sizeof (fromNode->game_ID), 0);
}

// handles disconnect
static void handle_disconnect(node *lobby, int cfd){
    int game_id;

    if(find_remaining_by_id(&lobby, cfd) == 1){
            insert_new_game(&lobby, lobby->game_id, lobby->game_env.fd_client_X, lobby->game_env.fd_client_O);
            send_new_game_data(&lobby->game_env, game_id);
            print(&lobby);
            reset_lobby(&lobby);
        }
    }

#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>




// todo make a .h file
#include "v2_FSM.c"

#include "shared.h"

int prepare_server();


typedef struct {
    int game_id;
    GameEnvironment game_env;

} SingleGame;

typedef struct {
    int x_fd;
    int o_fd;
    int game_id;
} GameLobby;


static void create_new_game(GameLobby *lobby, GameEnvironment *new_env);


int main() {
    fd_set rfds, ready_sockets;
    int retval, game_id ;
    int sfd = prepare_server();

    SingleGame gameCollection[10];
    GameLobby lobby;
    game_id = 0;
    lobby.x_fd = 0;

    FD_ZERO(&rfds);
    FD_SET((u_int) sfd, &rfds);

    while (1) {
        ssize_t num_read;
        char buf[10];
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
                    cfd = accept4(sfd, NULL, NULL, SOCK_NONBLOCK);
                    FD_SET((u_int) cfd, &rfds);
                    // if there is not x in the queue then new connect is player x
                    if (lobby.x_fd == 0) {
                        lobby.x_fd = cfd;
                        // send the assignmennt to the clientAreas
                    } else {
                        lobby.o_fd = cfd;
                        lobby.game_id = game_id;

                        GameEnvironment new_game;
                        create_new_game(&lobby, &new_game);
                        lobby.x_fd = 0;

                        SingleGame addToCollection;
                        addToCollection.game_env = new_game;
                        addToCollection.game_env.game_ID = game_id;
                        addToCollection.game_id = game_id;
                        gameCollection[game_id] = addToCollection;
                        game_id++;
                    }
                } else {
                    cfd = i;
                    uint8_t game;
                    num_read = read(cfd, &game, 1);
                    printf("GAME ID: %d\n", game);
                    read(cfd, &gameCollection[game].game_env.byte_input, sizeof (gameCollection[game].game_env.byte_input));
                    if (num_read < 1) {
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    }
                    mainaroo(&gameCollection[game].game_env);
                    printf("Exit FSM....\n");
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

// todo build a new game
//

static void create_new_game(GameLobby *lobby, GameEnvironment *new_env){

    new_env->fd_client_X = lobby->x_fd;
    new_env->fd_client_O = lobby->o_fd;
    new_env->fd_current_player = lobby->x_fd;
    new_env->current_player = X;
    for(int i = 0; i < 9; i++){
        new_env->game_board[i] = BLANK_SPACE;
    }

    uint8_t welcome = WELCOME;
    uint8_t assign_x = X;
    uint8_t assign_o = O;
    uint8_t invite = INVITE;

    send(new_env->fd_client_X, &welcome, sizeof (welcome), 0);
    send(new_env->fd_client_X, &assign_x, sizeof (assign_x), 0);
    send(new_env->fd_client_X, &lobby->game_id, sizeof (lobby->game_id), 0);
    send(new_env->fd_client_O, &welcome, sizeof (welcome), 0);
    send(new_env->fd_client_O, &assign_o, sizeof (assign_o), 0);
    send(new_env->fd_client_O, &lobby->game_id, sizeof (lobby->game_id), 0);

    printf("New GAME! \n");
}
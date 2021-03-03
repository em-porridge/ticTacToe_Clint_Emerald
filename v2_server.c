#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
                    cfd = accept(sfd, NULL, NULL);
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
                        addToCollection.game_id = game_id;
                        gameCollection[game_id] = addToCollection;
                        game_id++;
                        printf("CollectionID %d, CURRENT: %d\n", addToCollection.game_id,
                               addToCollection.game_env.fd_current_player);
                    }
                } else {
                    cfd = i;
                    num_read = read(cfd, buf, sizeof(buf));
                    if (num_read < 1) {
                        close(cfd);
                        FD_CLR((u_int) cfd, &rfds);
                    }
                    write(STDOUT_FILENO, buf, num_read);

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

    for(int i = 0; i < 9; i++){
        new_env->game_board[i] = BLANK_SPACE;
//        printf("GAME BOARD %d, %d\n",i,  game_env->game_board[i]);
    }

    uint8_t welcome = WELCOME;
    uint8_t assign_x = X;
    uint8_t assign_o = O;
    uint8_t invite = INVITE;

//
//    send(new_env->fd_client_O, &lobby->game_id, sizeof (lobby->game_id), 0);
    // not safe
    send(new_env->fd_client_X, &welcome, sizeof (welcome), 0);
    send(new_env->fd_client_X, &lobby->game_id, sizeof (lobby->game_id), 0);
    send(new_env->fd_client_X, &assign_x, sizeof (assign_x), 0);
    send(new_env->fd_client_O, &welcome, sizeof (welcome), 0);
    send(new_env->fd_client_O, &lobby->game_id, sizeof (lobby->game_id), 0);
    send(new_env->fd_client_O, &assign_o, sizeof (assign_o), 0);

//    send(new_env->fd_client_X, &invite, sizeof(invite), 0 );

////
//    send(new_env->fd_client_X, &lobby->game_id, sizeof (invite), 0);
//    for (int i = 0; i < 9; i++) {
//        uint8_t play = new_env->game_board[i];
//        send(new_env->fd_current_player, &play, sizeof (play), 0);
//    }

    printf("New GAME! \n");

}
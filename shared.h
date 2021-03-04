#ifndef SHARED_H
#define SHARED_H
#include <stdbool.h>
#include "fsm.h"

// https://www.sciencedirect.com/topics/computer-science/registered-port#:~:text=Ports%200%20through%201023%20are,be%20used%20dynamically%20by%20applications.
// /etc/services
#define PORT 65432
#define BACKLOG 5

#define BLANK_SPACE 45

// response codes
#define WELCOME 1
#define INVITE 4
#define INVALID 5
#define ACCEPTED 6
#define WIN 7
#define LOSE 8
#define TIE 9
#define DISCONNECT 10

// randoms to deal with ASCII conversions
#define OFFSET 48
#define LOWER_0 47
#define UPPER_8 57

#define X 88
#define O 79


typedef struct
{
    Environment common;
    int current_player;
    int game_ID;

    // file descriptors
    int fd_current_player;
    int fd_client_X;
    int fd_client_O;

    // for getting player move
    int8_t byte_input;
    int8_t response_type;
    int8_t winner;

    // for storing
    int game_board[9];
    int received_position;
    int play_count;
    bool game_over;

    char one_byte;
} GameEnvironment;


typedef struct {
    int game_id;
    GameEnvironment game_env;

} SingleGame;

typedef struct {
    int x_fd;
    int o_fd;
    int game_id;
    bool new_game;
    int existing_game_id;
} GameLobby;


#endif

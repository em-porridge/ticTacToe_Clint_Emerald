#ifndef SHARED_H
#define SHARED_H
#include <stdbool.h>
#include "fsm.h"

// https://www.sciencedirect.com/topics/computer-science/registered-port#:~:text=Ports%200%20through%201023%20are,be%20used%20dynamically%20by%20applications.
// /etc/services
#define PORT 42069
#define BACKLOG 5

#define BLANK_SPACE 45

// response codes
//#define WELCOME 1
//#define INVITE 4
//#define INVALID 5
//#define ACCEPTED 6
//#define WIN 7
//#define LOSE 8
//#define TIE 9
//#define DISCONNECT 10

// randoms to deal with ASCII conversions
//#define OFFSET 48
//#define LOWER_0 47
//#define UPPER_8 57

//#define X 88
//#define O 79

// CODES V3

// Request Codes
#define CONFIRMATION 1
#define INFORMATION 2
#define META_ACTION 3
#define GAME_ACTION 4

// Response Codes -- SERVER CONFIRMATION
#define SUCCESS 10
#define UPDATE 20
#define CLIENT_ERROR_INVALID_TYPE 30
#define CLIENT_ERROR_INVALID_CONTEXT 31
#define CLIENT_ERROR_INVALID_PAYLOAD 32
#define SERVER_ERROR 40
#define GAME_ERROR_INVALID_ACTION 50
#define GAME_ERROR_ACTION_OUT_OF_TURN 51

// Game State Codes
#define UPDATE_START_GAME 1
#define UPDATE_MOVE_MADE 2
#define UPDATE_END_OF_GAME 3
#define UPDATE_DISCONNECT 4
#define SUCCESS_CONFIRMATION 1

// Payload Value
#define TTT_GAME 1
#define RPS_GAME 2

#define PL_WIN 1
#define PL_LOSS 2
#define PL_TIE 3

#define X 1
#define O 2

#define ROCK 1
#define PAPER 2
#define SCISSORS 3

typedef struct {
    int32_t uid;
    int8_t req_type;
    int8_t context;
    int8_t payload_length;

    int8_t payload_first_byte;
    int8_t payload_second_byte;
    int8_t payload_third_byte;

} data ;


/** A singular Rock Paper Scissors game envirnoment **/
typedef struct {
    Environment common;
    // unique ID

    int32_t unique_game_id;

    data FSM_data_reads;

    int fd_client_player_one;
    int fd_client_player_two;

    int8_t client_one_play;
    int8_t client_two_play;

    bool game_over;

    // managing req/responses
    int8_t received_move;
    int8_t response_type;
    int8_t response_game_state;
    int8_t winner;
} SingleRPSGameEnv;

/** A singular TicTacToe game environment **/
typedef struct{
    Environment common;

    // ttt or rps
    int game_type;

    // unique ID
    int32_t client_x_uid;
    int32_t client_o_uid;

    int client_x;
    int client_o;
    int fd_current_player;

    //todo set these
    // for gameplay

    // ttt specific
    int game_board[9];
    int play_count;
    bool game_over;

    // managing req/responses
    int8_t received_move;
    int8_t winner;

} SingleTTTGameEnv;

#endif

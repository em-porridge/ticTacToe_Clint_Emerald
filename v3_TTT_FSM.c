//
// Created by emu on 2021-04-07.
//


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include "fsm.h"
#include "shared.h"
#include "v3_TTT_FSM.h"


// driver for turn FSM
int mainaroo(SingleTTTGameEnv *env)
{
    StateTransition transitions[] =
            {
                    {FSM_INIT, VALIDATE, &validate_input},
                    {VALIDATE, WAIT, &invalid_move},
                    {VALIDATE, GAMEOVER, &game_over},
                    {VALIDATE, TURNOVER, &accepted_move},
                    {WAIT, VALIDATE, &validate_input},
                    {TURNOVER, FSM_EXIT, &terminate},
                    {GAMEOVER, FSM_EXIT, &terminate}
            };

    int code;
    int start_state;
    int end_state;

    start_state = FSM_INIT;
    end_state   = VALIDATE;

    code = fsm_run((Environment *)env, &start_state, &end_state, transitions);

    if(code != 0)
    {
        fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Exiting state: %d\n", start_state);
    return EXIT_SUCCESS;
}



// validates input
static int validate_input(Environment *env){

    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    // stored payload should be a cell // todo change the 1 in the game env ICK !
    if(game_env->payload_1_byte >= 0 && game_env->payload_1_byte <= 8) {
        if(check_board(env)) {
            accepted_move(env);
        } else {
            //todo confirm this correct
            uint8_t status = GAME_ERROR_INVALID_ACTION;
            uint8_t req_type = INFORMATION;
            uint8_t payload_length = 0;

            // send invalid deets
            send(game_env->fd_current_player, &status, sizeof (status));
            send(game_env->fd_current_player, &req_type, sizeof (status));
            send(game_env->fd_current_player, &req_type, sizeof (status));

            return WAIT;
        }
    } else {
        uint8_t status = GAME_ERROR_INVALID_ACTION;
        uint8_t req_type = INFORMATION;
        uint8_t payload_length = 0;

        send(game_env->fd_current_player, &status, sizeof (status));
        send(game_env->fd_current_player, &req_type, sizeof (status));
        send(game_env->fd_current_player, &req_type, sizeof (status));
        return WAIT;
    }

    return TURNOVER;
}

// Read new client move -- "error check" lawl -- send to VALIDATE state
static int invalid_move(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *)env;

    //todo error check all these sweet potatoes
    int32_t uid;
    int8_t req_type;
    int8_t payload_length;
    int8_t payload;

    read(game_env->fd_current_player, &uid, sizeof (uid));
    read(game_env->fd_current_player, &req_type, sizeof (req_type));
    read(game_env->fd_current_player, &payload_length, sizeof (payload_length));
    read(game_env->fd_current_player, &payload, sizeof (req_type));

    game_env->received_move = payload;

    return VALIDATE;
}

// Accepted Move -- Switches Players and sends new player the deets
static int accepted_move(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *)env;

    // todo is this protocol?
    //    send approval to current player -- req_type --> payload_length --> payload
    //    send(game_env->fd_current_player, &game_env->req_type, sizeof (game_env->req_type), 0);

    switch_players(env);

//    send update --> status --> update_context --> payload_length --> payload
    uint8_t update = UPDATE;
    uint8_t move_made = UPDATE_MOVE_MADE;
    uint8_t payload_size = 1;
    uint8_t position = game_env->received_move;

    send(game_env->fd_current_player, &update, sizeof (update));
    send(game_env->fd_current_player, &move_made, sizeof (move_made));
    send(game_env->fd_current_player, &payload_size, sizeof (payload_size));
    send(game_env->fd_current_player, &position, sizeof (position));

    return FSM_EXIT;
}

// todo update
static bool check_board(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    if(game_env->game_board[game_env->received_move] == BLANK_SPACE){
        game_env->game_board[game_env->received_move] = game_env->fd_current_player;
//        game_env->response_type = SUCCESS;
        return true;
    } else {
//        game_env->response_type = CLIENT_ERROR_INVALID_TYPE;
        return false;
    }
}


static void print_board(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    for(int i = 0 ; i < 9; i++){
        if(game_env->game_board[i] != BLANK_SPACE){
            printf("| %c |", game_env->game_board[i]);
        } else {
            printf("|   |");
        }
    }
    printf("\n\n");
}

static void switch_players(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    game_env->play_count++;

    if(game_env->fd_current_player == game_env->client_x) {
        int swap_x = game_env->client_o;
        game_env->fd_current_player = swap_x;
    } else if(game_env->fd_current_player == game_env->client_o) {
        int swap_o = game_env->client_x;
        game_env->fd_current_player = swap_o;
    }
}

static bool check_win(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    uint8_t status = UPDATE;
    uint8_t context = UPDATE_END_OF_GAME;
    uint8_t payload_length = 2;
    uint8_t winner_code = PL_WIN;
    uint8_t loser_code_boo = PL_LOSS;
    uint8_t tie_code = PL_TIE;
    uint8_t final_move = game_env->received_move;


    if(check_rows_for_winner(env) | check_col_for_winner(env) | check_diagonal_for_winner(env)){
        printf("Game Ending Move! \n");

        if(game_env->client_x == game_env->winner) {
            // send win to x
            send(game_env->client_x, &status, sizeof (status));
            send(game_env->client_x, &context, sizeof (context));
            send(game_env->client_x, &payload_length, sizeof (payload_length));
            send(game_env->client_x, &winner_code, sizeof (winner_code));
            send(game_env->client_x, &final_move, sizeof (final_move))

            send(game_env->client_o, &status, sizeof (status));
            send(game_env->client_o, &context, sizeof (context));
            send(game_env->client_o, &payload_length, sizeof (payload_length));
            send(game_env->client_o, &loser_code_boo_code, sizeof (winner_code));
            send(game_env->client_o, &final_move, sizeof (final_move))
        } else {
            // send win to o
            send(game_env->client_x, &status, sizeof (status));
            send(game_env->client_x, &context, sizeof (context));
            send(game_env->client_x, &payload_length, sizeof (payload_length));
            send(game_env->client_x, &loser_code_boo, sizeof (winner_code));
            send(game_env->client_x, &final_move, sizeof (final_move))

            send(game_env->client_o, &status, sizeof (status));
            send(game_env->client_o, &context, sizeof (context));
            send(game_env->client_o, &payload_length, sizeof (payload_length));
            send(game_env->client_o, &winner_code, sizeof (winner_code));
            send(game_env->client_o, &final_move, sizeof (final_move))
        }
        return true;
    } else if (game_env->play_count == 9){
//        game_env->winner = BLANK_SPACE;
        send(game_env->client_x, &status, sizeof (status));
        send(game_env->client_x, &context, sizeof (context));
        send(game_env->client_x, &payload_length, sizeof (payload_length));
        send(game_env->client_x, &tie_code, sizeof (winner_code));
        send(game_env->client_x, &final_move, sizeof (final_move))

        send(game_env->client_o, &status, sizeof (status));
        send(game_env->client_o, &context, sizeof (context));
        send(game_env->client_o, &payload_length, sizeof (payload_length));
        send(game_env->client_o, &tie_code, sizeof (winner_code));
        send(game_env->client_o, &final_move, sizeof (final_move))

        // send tie to
        printf("Tie Game! \n");
        return true;
    } else {
        printf("No win detected \n");
        return false;
    }
}

static int check_rows_for_winner(Environment *env) {
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    int space_one;
    int space_two;
    int space_three;

    for(int i = 0; i < 9 ; i+=3){
        space_one = game_env->game_board[i];
        space_two = game_env->game_board[i+1];
        space_three = game_env->game_board[i+2];
        if(space_one == space_two && space_three == space_two && space_one != 45){
            game_env->winner = space_one;
            return true;
        }
    }
    return false;
}

static int check_col_for_winner(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    int space_one;
    int space_two;
    int space_three;

    for(int i = 0; i < 3; i++){
        space_one = game_env->game_board[i];
        space_two = game_env->game_board[i+3];
        space_three = game_env->game_board[i+6];
        if(space_one == space_two && space_three == space_two && space_one != 45){
            game_env->winner = space_one;
            return true;
        }
    }
    return false;
}

static int check_diagonal_for_winner(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    int space_one;
    int space_two;
    int space_three;
    int space_four;
    int space_five;

    space_one = game_env->game_board[0];
    space_two = game_env->game_board[4];
    space_three = game_env->game_board[8];
    space_four = game_env->game_board[2];
    space_five = game_env->game_board[6];

    if(space_one == space_two && space_two == space_three && space_two != 45) {
        game_env->winner = space_one;
        return true;
    } else if(space_four == space_two && space_two == space_five && space_two != 45) {
        game_env->winner = space_one;
        return true;
    } else {
        return false;
    }
}


static int game_over(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (GameEnvironment *) env;

    game_env->game_over = true;
    return FSM_EXIT;
}

static int terminate(Environment *env){
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}
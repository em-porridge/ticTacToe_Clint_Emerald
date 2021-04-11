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
    game_env = (SingleTTTGameEnv *) env;

    if(game_env->received_move >= 0 && game_env->received_move <= 8) {
        if(check_board(env)) {
            accepted_move(env);
            if(check_win(env)){
                return TURNOVER;
            }
        } else {
            invalid_move(env);
            return WAIT;
        }
    } else {
        send_decline_move_code(env);
        return WAIT;
    }

    return TURNOVER;
}

// Read new client move -- "error check" lawl -- send to VALIDATE state
static int invalid_move(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;
    send_decline_move_code(env);
    return VALIDATE;
}

// Accepted Move -- Switches Players and sends new player the deets
static int accepted_move(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    send_accepted_move_code(env);
    switch_players(env);
    send_update_client_code(env);
    return FSM_EXIT;
}

static bool check_board(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    if(game_env->game_board[game_env->received_move] == BLANK_SPACE){
        game_env->game_board[game_env->received_move] = game_env->fd_current_player;
        return true;
    } else {
        return false;
    }
}


static void print_board(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

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
    game_env = (SingleTTTGameEnv *) env;

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
    game_env = (SingleTTTGameEnv *) env;

    if(check_rows_for_winner(env) | check_col_for_winner(env) | check_diagonal_for_winner(env)){
        send_win_loss_game(env);
        printf("Game Ending Move! \n");
        return true;
    } else if (game_env->play_count == 9){
        send_tie_game(env);
        printf("Tie Game! \n");
        return true;
    } else {
        printf("No win detected \n");
        return false;
    }
}

static int check_rows_for_winner(Environment *env) {
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

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
    game_env = (SingleTTTGameEnv *) env;

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
    game_env = (SingleTTTGameEnv *) env;

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
    game_env = (SingleTTTGameEnv *) env;

    game_env->game_over = true;
    return FSM_EXIT;
}

static int terminate(Environment *env){
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}


static int send_update_client_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    int8_t status = UPDATE;
    int8_t context = UPDATE_MOVE_MADE;
    int8_t payload_length = 1;
    int8_t payload = game_env->received_move;

    send(game_env->fd_current_player, &status, sizeof (status), 0);
    send(game_env->fd_current_player, &context, sizeof (context), 0);
    send(game_env->fd_current_player, &payload_length, sizeof (payload_length), 0);
    send(game_env->fd_current_player, &payload, sizeof (payload), 0);

    return 0;
}


static int send_accepted_move_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    uint8_t status = SUCCESS;
    uint8_t context = INFORMATION;
    uint8_t payload_size = 0;

    send(game_env->fd_current_player, &status, sizeof (status), 0);
    send(game_env->fd_current_player, &context, sizeof (context), 0);
    send(game_env->fd_current_player, &payload_size, sizeof (payload_size), 0);
}


static int send_decline_move_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    uint8_t status = GAME_ERROR_INVALID_ACTION;
    uint8_t context = INFORMATION;
    uint8_t payload_length = 0;

    send(game_env->fd_current_player, &status, sizeof (status), 0);
    send(game_env->fd_current_player, &context, sizeof (context), 0);
    send(game_env->fd_current_player, &payload_length, sizeof (payload_length), 0);

    return 0;
}

static int send_tie_game(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    uint8_t status = UPDATE;
    uint8_t context = UPDATE_END_OF_GAME;
    uint8_t payload_length = 2;
    uint8_t tie_code = PL_TIE;
    uint8_t final_move = game_env->received_move;

    send(game_env->client_x, &status, sizeof (status), 0);
    send(game_env->client_x, &context, sizeof (context), 0);
    send(game_env->client_x, &payload_length, sizeof (payload_length), 0);
    send(game_env->client_x, &tie_code, sizeof (tie_code), 0);
    send(game_env->client_x, &final_move, sizeof (final_move), 0);

    send(game_env->client_o, &status, sizeof (status), 0);
    send(game_env->client_o, &context, sizeof (context), 0);
    send(game_env->client_o, &payload_length, sizeof (payload_length), 0);
    send(game_env->client_o, &tie_code, sizeof (tie_code), 0);
    send(game_env->client_o, &final_move, sizeof (final_move), 0);

    return 0;
}

static int send_win_loss_game(Environment *env) {
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    uint8_t status = UPDATE;
    uint8_t context = UPDATE_END_OF_GAME;
    uint8_t payload_length = 2;
    uint8_t winner_code = PL_WIN;
    uint8_t loser_code_boo = PL_LOSS;
    uint8_t final_move = game_env->received_move;


    if(game_env->client_x == game_env->winner) {
        // send win to x
        send(game_env->client_x, &status, sizeof (status), 0);
        send(game_env->client_x, &context, sizeof (context), 0);
        send(game_env->client_x, &payload_length, sizeof (payload_length), 0);
        send(game_env->client_x, &winner_code, sizeof (winner_code), 0);
        send(game_env->client_x, &final_move, sizeof (&game_env->received_move), 0);

        send(game_env->client_o, &status, sizeof (status), 0);
        send(game_env->client_o, &context, sizeof (context), 0);
        send(game_env->client_o, &payload_length, sizeof (payload_length), 0);
        send(game_env->client_o, &loser_code_boo, sizeof (winner_code), 0);
        send(game_env->client_o, &final_move, sizeof (&final_move), 0);
    } else if(game_env->client_o == game_env->winner) {
        // send win to o
        send(game_env->client_x, &status, sizeof (status), 0);
        send(game_env->client_x, &context, sizeof (context), 0);
        send(game_env->client_x, &payload_length, sizeof (payload_length), 0);
        send(game_env->client_x, &loser_code_boo, sizeof (winner_code), 0);
        send(game_env->client_x, &final_move, sizeof (final_move), 0);

        send(game_env->client_o, &status, sizeof (status), 0);
        send(game_env->client_o, &context, sizeof (context), 0);
        send(game_env->client_o, &payload_length, sizeof (payload_length), 0);
        send(game_env->client_o, &winner_code, sizeof (winner_code), 0);
        send(game_env->client_o, &final_move, sizeof (final_move), 0);
    } else {
        // todo server error
    }
    return 1;
}
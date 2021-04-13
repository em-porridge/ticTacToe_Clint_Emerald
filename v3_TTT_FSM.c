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
//            accepted_move(env);
            if(check_win(env)){
                return GAMEOVER;
            }
        } else {
            invalid_move(env);
            return WAIT;
        }
    } else {
        invalid_move(env);
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
    print_board(env);
    switch_players(env);
    send_update_client_code(env);
    return FSM_EXIT;
}

static int game_over(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->game_over = true;
    return FSM_EXIT;
}

// =========== HELPER FUNCTIONS ================== //

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
            printf("| %d |", game_env->game_board[i]);
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
        game_env->fd_current_player = game_env->client_o;
    } else if(game_env->fd_current_player == game_env->client_o) {
        game_env->fd_current_player = game_env->client_x;
    } else {
        // i dunno
    }
}

static bool check_win(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    if(check_rows_for_winner(env) | check_col_for_winner(env) | check_diagonal_for_winner(env)){
        send_win_loss_game(env);
        printf("Game Ending Move! \n");
        return true;
    } else if (game_env->play_count == 8){
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

static int terminate(Environment *env){
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}

//===============CODES===============//

static int send_update_client_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->FSM_data_reads.req_type = UPDATE;
    game_env->FSM_data_reads.context = UPDATE_MOVE_MADE;
    game_env->FSM_data_reads.payload_length = 1;
    game_env->FSM_data_reads.payload_first_byte = game_env->received_move;

    unsigned char byte_array_update[4];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;
    byte_array_update[3] = game_env->received_move;

    send(game_env->fd_current_player, &byte_array_update, sizeof (byte_array_update), 0);

    return 0;
}


static int send_accepted_move_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->FSM_data_reads.req_type = SUCCESS;
    game_env->FSM_data_reads.context = INFORMATION;
    game_env->FSM_data_reads.payload_length = 0;

    unsigned char byte_array_update[3];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;


    send(game_env->fd_current_player, &byte_array_update, sizeof (byte_array_update), 0);
}


static int send_decline_move_code(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->FSM_data_reads.req_type = GAME_ERROR_INVALID_ACTION;
    game_env->FSM_data_reads.context = INFORMATION;
    game_env->FSM_data_reads.payload_length = 0;

    unsigned char byte_array_update[3];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;

    send(game_env->fd_current_player, &byte_array_update, sizeof (byte_array_update), 0);
    return 0;
}

static int send_tie_game(Environment *env){
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->FSM_data_reads.req_type = UPDATE;
    game_env->FSM_data_reads.context = UPDATE_END_OF_GAME;
    game_env->FSM_data_reads.payload_length = 2;
    game_env->FSM_data_reads.payload_first_byte = PL_TIE;
    game_env->FSM_data_reads.payload_second_byte = game_env ->received_move;

    unsigned char byte_array_update[6];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;
    byte_array_update[3] = game_env->FSM_data_reads.payload_first_byte;
    byte_array_update[4] = game_env->FSM_data_reads.payload_second_byte;

    send_accepted_move_code(env);
    send(game_env->client_x, &byte_array_update, sizeof (byte_array_update), 0);
    send(game_env->client_o, &byte_array_update, sizeof (byte_array_update), 0);

    return 0;
}

static int send_win_loss_game(Environment *env) {
    SingleTTTGameEnv *game_env;
    game_env = (SingleTTTGameEnv *) env;

    game_env->FSM_data_reads.req_type = UPDATE;
    game_env->FSM_data_reads.context = UPDATE_END_OF_GAME;
    game_env->FSM_data_reads.payload_length = 2;
    game_env->FSM_data_reads.payload_first_byte = PL_WIN;
    game_env->FSM_data_reads.payload_second_byte = PL_LOSS;
    game_env->FSM_data_reads.payload_third_byte = game_env ->received_move;

    unsigned char byte_array_update_winner[6];
    unsigned char byte_array_update_loser[6];

    byte_array_update_winner[0] = game_env->FSM_data_reads.req_type;
    byte_array_update_winner[1] = game_env->FSM_data_reads.context;
    byte_array_update_winner[2] = game_env->FSM_data_reads.payload_length;
    byte_array_update_winner[3] = game_env->FSM_data_reads.payload_first_byte;
    byte_array_update_winner[4] = game_env->FSM_data_reads.payload_third_byte;

    byte_array_update_loser[0] = game_env->FSM_data_reads.req_type;
    byte_array_update_loser[1] = game_env->FSM_data_reads.context;
    byte_array_update_loser[2] = game_env->FSM_data_reads.payload_length;
    byte_array_update_loser[3] = game_env->FSM_data_reads.payload_second_byte;
    byte_array_update_loser[4] = game_env->FSM_data_reads.payload_third_byte;

    if(game_env->client_x == game_env->winner) {
        // send win to x
        send_accepted_move_code(env);
        send(game_env->client_x, &byte_array_update_winner, sizeof (byte_array_update_winner), 0);
        send(game_env->client_o, &byte_array_update_loser, sizeof (byte_array_update_winner), 0);
    } else if(game_env->client_o == game_env->winner) {
        // send win to o
        send_accepted_move_code(env);
        send(game_env->client_x, &byte_array_update_loser, sizeof (byte_array_update_loser), 0);
        send(game_env->client_o, &byte_array_update_winner, sizeof (byte_array_update_winner), 0);
    } else {
        // todo server error
    }
    return 1;
}
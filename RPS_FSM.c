//
// Created by emu on 2021-04-12.
//


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include "fsm.h"
#include "shared.h"
#include "RPS_FSM.h"

int mainzees(SingleRPSGameEnv *env)
{
    StateTransition transitions[] =
            {
                    {FSM_INIT, RPS_VALIDATE, &validate_RPS_input},
                    {RPS_VALIDATE, RPS_WAIT, &invalid_RPS_move},
                    {RPS_VALIDATE, RPS_GAMEOVER, &game_RPS_over},
                    {RPS_VALIDATE, RPS_TURNOVER, &accepted_RPS_move},
                    {RPS_WAIT, RPS_VALIDATE, &validate_RPS_input},
                    {RPS_TURNOVER, FSM_EXIT, &RPS_terminate},
                    {RPS_GAMEOVER, FSM_EXIT, &RPS_terminate}
            };

    int code;
    int start_state;
    int end_state;

    start_state = FSM_INIT;
    end_state   = RPS_VALIDATE;

    code = fsm_run((Environment *)env, &start_state, &end_state, transitions);

    if(code != 0)
    {
        fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Exiting state: %d\n", start_state);
    return EXIT_SUCCESS;
}


static int validate_RPS_input(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    if(!check_player_gone(env)) {
        if(check_RPS_move_valid(env)){
            if(check_RPS_game_over(env)){
                return RPS_GAMEOVER;
            } else {
                return RPS_TURNOVER;
            }
        } else {
            return RPS_WAIT;
        }
    }
    // read
    // check input

}

static int invalid_RPS_move(Environment *env) {
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;
    send_RPS_decline_play_code(env);

};

static int accepted_RPS_move(Environment *env) {
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;
    send_accept_play_code(env);
    return FSM_EXIT;
};

static int game_RPS_over(Environment *env) {
    return FSM_EXIT;
};

static int RPS_terminate(Environment *env) {
    printf("Terminating...");
    exit(EXIT_FAILURE);
};


static int check_player_gone(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;
    if(game_env->fd_current_client == game_env->fd_client_player_one) {
        if(game_env->client_one_play == 0){
            return false;
        } else {
            // send error
            return true;
        }
    } else if (game_env -> fd_current_client == game_env ->fd_client_player_two) {
        if(game_env->client_two_play == 0) {
            game_env->client_two_play = game_env->move_received;
            return  false;
        } else {
            return true;
        }
    } else {
        // error
    }
    return false;
}


static int check_RPS_move_valid(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;
    if(game_env->move_received > 0 && game_env->move_received <= 3) {
        return true;
    } else {
        return false;
    }
}

static int check_RPS_game_over(Environment *env) {
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    if(game_env->client_two_play > 0 && game_env->client_one_play > 0 ) {
        int winner = ((3 + (game_env->client_one_play)-(game_env->client_two_play)) % 3);

        if(winner == 0) {
            send_RPS_tie_game(env);
        } else if(winner == 1) {
            game_env->winner = game_env->fd_client_player_one;
            send_RPS_win_loss_game_codes(env);
        } else if (winner == 2) {
            game_env->winner = game_env->fd_client_player_two;
            send_RPS_win_loss_game_codes(env);
        }
        return true;
    }
    return false;
}

static int send_accept_play_code(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    game_env->FSM_data_reads.req_type = SUCCESS;
    game_env->FSM_data_reads.context = INFORMATION;
    game_env->FSM_data_reads.payload_length = 0;

    unsigned char byte_array_update[3];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;

    send(game_env->fd_current_client, &byte_array_update, sizeof (byte_array_update), 0);
}


static int send_RPS_decline_play_code(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    game_env->FSM_data_reads.req_type = GAME_ERROR_INVALID_ACTION;
    game_env->FSM_data_reads.context = INFORMATION;
    game_env->FSM_data_reads.payload_length = 0;

    unsigned char byte_array_update[3];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;

    send(game_env->fd_current_client, &byte_array_update, sizeof (byte_array_update), 0);
    return 0;
}

static int send_RPS_win_loss_game_codes(Environment *env) {
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    game_env->FSM_data_reads.req_type = UPDATE;
    game_env->FSM_data_reads.context = UPDATE_END_OF_GAME;
    game_env->FSM_data_reads.payload_length = 2;
    game_env->FSM_data_reads.payload_first_byte = PL_WIN;
    game_env->FSM_data_reads.payload_second_byte = PL_LOSS;
    game_env->FSM_data_reads.payload_third_byte = game_env->move_received;

    unsigned char byte_array_update_winner[5];
    unsigned char byte_array_update_loser[5];

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

    if(game_env->client_one_play == game_env->winner) {
        // send win to x
        send_accept_play_code(env);
        send(game_env->client_one_play, &byte_array_update_winner, sizeof (byte_array_update_winner), 0);
        send(game_env->client_two_play, &byte_array_update_loser, sizeof (byte_array_update_winner), 0);
    } else if(game_env->client_two_play == game_env->winner) {
        // send win to o
        send_accept_play_code(env);
        send(game_env->client_two_play, &byte_array_update_loser, sizeof (byte_array_update_loser), 0);
        send(game_env->client_one_play, &byte_array_update_winner, sizeof (byte_array_update_winner), 0);
    } else {
        // todo server error
    }
    return 1;
}


static int send_RPS_tie_game(Environment *env){
    SingleRPSGameEnv *game_env;
    game_env = (SingleRPSGameEnv *) env;

    game_env->FSM_data_reads.req_type = UPDATE;
    game_env->FSM_data_reads.context = UPDATE_END_OF_GAME;
    game_env->FSM_data_reads.payload_length = 2;
    game_env->FSM_data_reads.payload_first_byte = PL_TIE;
    game_env->FSM_data_reads.payload_second_byte = game_env->move_received;

    unsigned char byte_array_update[6];

    byte_array_update[0] = game_env->FSM_data_reads.req_type;
    byte_array_update[1] = game_env->FSM_data_reads.context;
    byte_array_update[2] = game_env->FSM_data_reads.payload_length;
    byte_array_update[3] = game_env->FSM_data_reads.payload_first_byte;
    byte_array_update[4] = game_env->FSM_data_reads.payload_second_byte;

    send_accept_play_code(env);
    send(game_env->fd_client_player_one, &byte_array_update, sizeof (byte_array_update), 0);
    send(game_env->fd_client_player_two, &byte_array_update, sizeof (byte_array_update), 0);

    return 0;
}
//
// Created by emu on 2021-04-12.
//

#ifndef A01_CLINT_EMERALD_RPS_FSM_H
#define A01_CLINT_EMERALD_RPS_FSM_H

typedef enum
{
    INIT,
    WAIT, // 4
    VALIDATE, // 5
    TURNOVER, // 6
    GAMEOVER, // 7
    ERROR, // 8

} States;


int mainzees(SingleRPSGameEnv *env);

static int validate_RPS_input(Environment *env);
static int invalid_RPS_move(Environment *env);
static int accepted_RPS_move(Environment *env);
static int game_RPS_over(Environment *env);
static int RPS_terminate(Environment *env);

static int check_player_gone(Environment *env);
static int check_RPS_move_valid(Environment *env);
static int check_RPS_game_over(Environment *env);
static int check_RPS_winner(Environment *env);

static int send_RPS_tie_game(Environment *env);
static int send_RPS_win_loss_game_codes(Environment *env);
static int send_accept_play_code(Environment *env);


#endif //A01_CLINT_EMERALD_RPS_FSM_H

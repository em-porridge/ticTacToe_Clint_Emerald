//
// Created by emu on 2021-04-07.
//
#include "shared.h"
#include "fsm.h"

#ifndef A01_CLINT_EMERALD_V3_TTT_FSM_H
#define A01_CLINT_EMERALD_V3_TTT_FSM_H

typedef enum
{
    INIT,
    WAIT, // 4
    VALIDATE, // 5
    TURNOVER, // 6
    GAMEOVER, // 7
    ERROR, // 8

} States;

int mainaroo( SingleTTTGameEnv *env);
//
static int validate_input(Environment *env);
static int invalid_move(Environment *env);
static int accepted_move(Environment *env);
static int game_over(Environment *env);
static int terminate(Environment *env);

// checking board
static bool check_board(Environment *env);
static void print_board(Environment *env);
static void switch_players(Environment *env);
static bool check_win(Environment *env);
static int check_rows_for_winner(Environment *env);
static int check_col_for_winner(Environment *env);
static int check_diagonal_for_winner(Environment *env);


// sending codes
static int send_update_client_code(Environment *env);
static int send_accepted_move_code(Environment *env);
static int send_decline_move_code(Environment *env);
static int send_tie_game(Environment *env);
static int send_win_loss_game(Environment *env);

#endif //A01_CLINT_EMERALD_V3_TTT_FSM_H

//
// Created by emu on 2021-03-04.
//
#include "../shared.h"

#ifndef A01_CLINT_EMERALD_V2_FSM_H
#define A01_CLINT_EMERALD_V2_FSM_H
typedef enum
{
    INIT,
    WAIT, // 4
    VALIDATE, // 5
    TURNOVER, // 6
    GAMEOVER, // 7
    ERROR, // 8

} States;

int mainaroo(GameEnvironment *env);
static int validate_input(Environment *env);
static int invalid_move(Environment *env);
static int accepted_move(Environment *env);
static bool check_board(Environment *env);
static void print_board(Environment *env);
static void switch_players(Environment *env);
static bool check_win(Environment *env);
static int check_rows_for_winner(Environment *env);
static int check_col_for_winner(Environment *env);
static int check_diagonal_for_winner(Environment *env);
static int game_over(Environment *env);
static int terminate(Environment *env);



#endif //A01_CLINT_EMERALD_V2_FSM_H

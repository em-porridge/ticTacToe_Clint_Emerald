
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include "fsm.h"
#include "v2_FSM.h"
#include "shared.h"


// driver for turn FSM
int mainaroo(GameEnvironment *env)
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
    printf("STARTING FSM \n CURRENT PLAYER: %d", env->fd_current_player);
    printf("\n RECEIVED FROM PLAYER: %u\n", env->byte_input);
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
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;
    // change name of one_byte, i hate it
    if(game_env->byte_input > LOWER_0 && game_env->byte_input < UPPER_8) {
        check_board(env);
        if(check_win(env)){
            return GAMEOVER;
        }
//        game_env->response_type = ACCEPTED;
        print_board(env);
    } else {
        game_env->response_type = INVALID;

        send(game_env->fd_current_player, &game_env->response_type, sizeof (game_env->response_type), 0);
        send(game_env->fd_current_player, &game_env->game_ID, sizeof (game_env->game_ID), 0);

        printf("not approved\n");
        return WAIT;
    }
    return TURNOVER;
}

// send re-request due to invalid move
static int invalid_move(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;
    // todo will probably have to read two bytes of info - gameID and number
    read(game_env->fd_current_player, &game_env->one_byte, 1);
    read(game_env->fd_current_player, &game_env->one_byte, 1);
    return VALIDATE;
}

// accepted move switch turns
static int accepted_move(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;
    
//    send(game_env->fd_current_player, &game_env->game_ID, sizeof (game_env->game_ID), 0);
    send(game_env->fd_current_player, &game_env->response_type, sizeof (game_env->response_type), 0);
//    for (int i = 0; i < 9; i++) {
//        uint8_t play = game_env->game_board[i];
//        send(game_env->fd_current_player, &play, sizeof (play), 0);
//    }

    switch_players(env);

    uint8_t invitation = INVITE;
    send(game_env->fd_current_player, &game_env->game_ID, sizeof (game_env->game_ID), 0);
    send(game_env->fd_current_player, &invitation, sizeof (invitation), 0);

    for (int i = 0; i < 9; i++) {
        uint8_t play = game_env->game_board[i];
        send(game_env->fd_current_player, &play, sizeof (play), 0);
    }

    return FSM_EXIT;
}

static bool check_board(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;

    game_env->received_position = (int) game_env->byte_input - OFFSET;

    if(game_env->game_board[game_env->received_position] == BLANK_SPACE){
        game_env->game_board[game_env->received_position] = game_env->current_player;
        game_env->response_type = ACCEPTED;
        return true;
    } else {
        // keep current player
        game_env->response_type = INVALID;
        return false;
    }
}

static void print_board(Environment *env){
    GameEnvironment *game_env;
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
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;

    game_env->play_count++;
    if(game_env->fd_current_player == game_env->fd_client_X){
        int swap_x = game_env->fd_client_O;
        game_env->fd_current_player = swap_x;
        game_env->current_player = O;
    } else if (game_env->fd_current_player == game_env->fd_client_O) {
        int swap_o = game_env->fd_client_X;
        game_env->fd_current_player = swap_o;
        game_env->current_player = X;
    }
}

static bool check_win(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;

    if(check_rows_for_winner(env) | check_col_for_winner(env) | check_diagonal_for_winner(env)){
        printf("Winning Move\n");
        return true;
    } else if (game_env->play_count == 9){
        game_env->winner = BLANK_SPACE;
        printf("Tie Game");
        return true;
    } else {
        printf("No win detected\n");
        return false;
    }
}

static int check_rows_for_winner(Environment *env) {
    GameEnvironment *game_env;
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
    GameEnvironment *game_env;
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
    GameEnvironment  *game_env;
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
    GameEnvironment  *game_env;
    game_env = (GameEnvironment *) env;

    int8_t winner = WIN;
    int8_t loser = LOSE;
    int8_t tie = TIE;

    if(game_env->winner == X) {
        write(game_env->fd_client_X, &winner, 1);
        write(game_env->fd_client_O, &loser, 1);
    } else if(game_env->winner == O){
        write(game_env->fd_client_O, &winner, 1);
        write(game_env->fd_client_X, &loser, 1);
    } else if(game_env->winner == BLANK_SPACE){
        write(game_env->fd_client_X, &tie, 1);
        write(game_env->fd_client_O, &tie, 1);
    }
    game_env->game_over = true;
    return FSM_EXIT;
}

static int terminate(Environment *env){
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}

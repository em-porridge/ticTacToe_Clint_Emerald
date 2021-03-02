#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include "fsm.h"
#include "shared.h"

// FSM functions
static int server_on(Environment *env);
static int send_assignment(Environment *env);
static int connect_player_o(Environment *env);
static int waiting_for_client(Environment *env);
static int validate_input(Environment *env);
static int send_response_to_client(Environment *env);
static int terminate(Environment *env);
static int game_over(Environment *env);
static int disconnect(Environment *env);


// helper functions
static bool check_board(Environment *env);
static void print_board(Environment *env);
static int check_rows_for_winner(Environment *env);
static int check_col_for_winner(Environment *env);
static int check_diagonal_for_winner(Environment *env);
static bool check_win(Environment *env);
static void switch_players(Environment *env);

typedef enum
{
    INIT,
    ACCEPT_PLAYER_X, // 1
    ACCEPT_PLAYER_O, // 2
    READY, // 3
    WAIT, // 4
    VALIDATE, // 5
    END, // 6
    GAMEOVER, // 7
    ERROR, // 8

} States;

// todo add a waiting player vs current player
typedef struct
{
    Environment common;

    // keep track of current player
    int current_player;
    // file descriptors
    int fd_current_player;
    int server_fd;
    int fd_client_X;
    int fd_client_O;
    // for getting player move
    int8_t byte_input;
    int8_t response_type;
    int8_t winner;

    // for storing current player
    int game_board[9];
    int received_position;
    int play_count;
    char one_byte;


} GameEnvironment;

int main(int argc, char *argv[])
{
    GameEnvironment env;
    StateTransition transitions[] =
            {
                    {FSM_INIT,ACCEPT_PLAYER_X, &server_on},
                    {ACCEPT_PLAYER_X, ERROR, &terminate},
                    {ACCEPT_PLAYER_X, ACCEPT_PLAYER_O, &connect_player_o},
                    {ACCEPT_PLAYER_O, READY, &send_assignment},
                    {READY, WAIT, &waiting_for_client},
                    {WAIT, FSM_EXIT, &terminate},
                    {WAIT, VALIDATE, &validate_input},
                    {WAIT, WAIT, &waiting_for_client},
                    {VALIDATE, WAIT, &send_response_to_client},
                    {VALIDATE, READY, &waiting_for_client},
                    {VALIDATE, GAMEOVER, &game_over},
                    {GAMEOVER, FSM_EXIT, &terminate},
                    {GAMEOVER, READY, &send_assignment},
                    {WAIT, ACCEPT_PLAYER_O, &connect_player_o}
            };

    int code;
    int start_state;
    int end_state;
    env.play_count = 0;

    start_state = FSM_INIT;
    end_state   = ACCEPT_PLAYER_X;


    code = fsm_run((Environment *)&env, &start_state, &end_state, transitions);


    if(code != 0)
    {
        fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Exiting state: %d\n", start_state);
    return EXIT_SUCCESS;
}

static int server_on(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;

    struct sockaddr_in sockaddr;
    int server_fd;

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Filed to create socket");
        exit(EXIT_FAILURE);
    }

    memset(&sockaddr, 0, sizeof(struct sockaddr_in));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *) &sockaddr, sizeof (sockaddr)) < 0) {
        perror("Failed to bind socket");
        return ERROR;
    }
    if(listen(server_fd, BACKLOG) < 0 ) {
        perror("Listening Error");
        return ERROR;
    }

    game_env->server_fd = server_fd;

    for (;;) {
        int client_fd;
        printf("Waiting for a connection");
        if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
            perror("Accept Error");
            return ERROR;
        }

        printf("\n\n =============================\n CONNECTION ACCEPTED \n=============================\n\n");

        game_env->fd_client_X = client_fd;
        game_env->fd_current_player = client_fd;
        game_env->current_player = X;
        return ACCEPT_PLAYER_O;
    }
};

static int connect_player_o(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;

    for (;;) {
        int client_fd;
        printf("Waiting for a connection");
        if ((client_fd = accept(game_env->server_fd, NULL, NULL)) < 0) {
            perror("Accept Error");
            // todo error state
            exit(EXIT_FAILURE);
        }
        printf("\n\n =============================\n CONNECTION ACCEPTED \n=============================\n\n");
        game_env->fd_client_O = client_fd;
        return READY;
    }
}

static int send_assignment(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;

    printf("Sending Assignment to client with FD: %u", game_env->fd_client_X);
    printf("Sending Assignment to client with FD: %u", game_env->fd_client_O);

    uint8_t welcome = WELCOME;
    uint8_t assign_x = X;
    uint8_t assign_o = O;

    send(game_env->fd_client_X, &welcome, sizeof (welcome), 0);
    send(game_env->fd_client_X, &assign_x, sizeof (assign_x), 0);
    send(game_env->fd_client_O, &welcome, sizeof (welcome), 0);
    send(game_env->fd_client_O, &assign_o, sizeof (assign_o), 0);

    // build board
    printf("\n");
    for(int i = 0; i < 9; i++){
        game_env->game_board[i] = BLANK_SPACE;
//        printf("GAME BOARD %d, %d\n",i,  game_env->game_board[i]);
    }

    return WAIT;
}


static int waiting_for_client(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;

    printf("WAITING.....\n");

    memset(&game_env->one_byte, '\0', sizeof (game_env->one_byte));

    if(read(game_env->fd_current_player, &game_env->one_byte, 1) < 0 | game_env->one_byte == 0){

        printf("=============== Client Disconnected ===============\n");

        if(game_env->fd_current_player == game_env->fd_client_X){
            game_env->fd_current_player = game_env->fd_client_O;
            close(game_env->fd_client_X);
            game_env->fd_client_X = game_env->fd_current_player;
//            close(game_env->fd_client_O);
        } else {
            game_env->fd_current_player = game_env->fd_client_X;
//            close(game_env->fd_client_O);
        }
        return ACCEPT_PLAYER_O;
    }
    printf(" PLAYED READ %c , %d", game_env->one_byte, game_env->one_byte);
    game_env->received_position = game_env->one_byte-OFFSET;
    return VALIDATE;
}

static int validate_input(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *)env;

    if(game_env->one_byte > LOWER_0 && game_env->one_byte < UPPER_8) {
        check_board(env);
        if(check_win(env)){
            return GAMEOVER;
        }
        print_board(env);
    } else {
        // change response type
        game_env->response_type = INVALID;
        printf("not approved\n");
    }
    return WAIT;
}


static bool check_board(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;
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

static int send_response_to_client(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;

    if(game_env->response_type == ACCEPTED){
        // todo send updated board to current client
        send(game_env->fd_current_player, &game_env->response_type, sizeof (game_env->response_type), 0);
        // send approval code to current client
        switch_players(env);
        // send invitaion to new client
        uint8_t invitation = INVITE;
        send(game_env->fd_current_player, &invitation, sizeof (invitation), 0);

	for (int i = 0; i < 9; i++) {
		uint8_t play = game_env->game_board[i];
        	send(game_env->fd_current_player, &play, sizeof (play), 0);
	}

    }
    if(game_env->response_type == INVALID){
        // stay on current client
        // send for a re-response
        send(game_env->fd_current_player, &game_env->response_type, sizeof (game_env->response_type), 0);
    }

    return WAIT;
}

// todo simplify
static void switch_players(Environment *env){
    GameEnvironment *game_env;
    game_env = (GameEnvironment *) env;

    game_env->play_count++;

    if(game_env->fd_current_player == game_env->fd_client_X){
        int swap_x = game_env->fd_client_O;
        game_env->fd_current_player = swap_x;
        game_env->current_player = O;
    } else if (game_env->fd_current_player == game_env->fd_client_O) {
        game_env->current_player = X;
        int swap_o = game_env->fd_client_X;
        game_env->fd_current_player = swap_o;
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
    return READY;
}

static int terminate(Environment *env){
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}

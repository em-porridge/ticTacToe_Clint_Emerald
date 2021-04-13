//
// Created by emu on 2021-04-08.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "GameCollection_LinkedList.h"

// initializes a new RSP Lobby
int init_lobbies(node **head){
    *head = malloc(sizeof (struct node));
    if (!*head) {
        fprintf(stderr, "Failed to init Lobby\n");
        return 1;
    }

    // A Rock Paper Scissors Lobby
    SingleRPSGameEnv rsp_lobby;
    rsp_lobby.unique_game_id_player_one = 0;
    rsp_lobby.unique_game_id_player_two = 0;
    rsp_lobby.fd_client_player_one = 0;
    rsp_lobby.fd_client_player_two = 0;
    (*head)->RPSGame = rsp_lobby;

    // A TTT Lobby
    SingleTTTGameEnv ttt_lobby;
    ttt_lobby.client_o_uid = 2;
    ttt_lobby.client_x_uid = 2;

    ttt_lobby.client_x = 0;
    ttt_lobby.client_o = 0;

    (*head)->TTTGame=ttt_lobby;

    (*head)->game_type = 0;
    (*head)->next = NULL;

    return 0;
};


int insert_new_ttt_game(node **head, SingleTTTGameEnv *new_env) {
    struct node *current = *head;
    struct node *tmp;

    do{
        tmp = current;
        current = current->next;
    } while (current);

    struct node *new_TTT_node = malloc(sizeof (struct node));

    if(!new_TTT_node) {
        fprintf(stderr, "Failed to insert a new TTT Game\n");
        return 1;
    }

    new_TTT_node->next = NULL;

    new_TTT_node->game_type = 1;
    new_TTT_node->uid_client_one = new_env->client_x_uid;
    new_TTT_node->uid_client_two = new_env->client_o_uid;
    new_TTT_node->TTTGame = *new_env;
    tmp->next = new_TTT_node;
    return 0;

}

int insert_new_rps_game(node **head, SingleRPSGameEnv *new_env){
    struct node *current = *head;
    struct node *tmp;

    do{
        tmp = current;
        current = current->next;
    } while (current);

    struct node *new_rsp_node = malloc(sizeof (struct node));
    if(!new_rsp_node) {
        fprintf(stderr, "Failed to insert a new RSP Game\n");
        return 1;
    }

    new_rsp_node->next = NULL;
    new_rsp_node->game_type = 2;

    new_rsp_node->uid_client_one = new_env->unique_game_id_player_one;
    new_rsp_node->uid_client_two = new_env->unique_game_id_player_two;

    new_rsp_node->RPSGame = *new_env;
    tmp->next = new_rsp_node;

}

node *return_link_by_uid(node ** head, uint32_t uid){
    struct node * current = *head;
    struct node * tmp;

    //todo update
    do {
        tmp = current;
        current = current->next;
    } while ((tmp->uid_client_one != uid || tmp->uid_client_two != uid) && current);

    if(tmp->uid_client_one == uid) {
        return tmp;
    } else if(tmp->uid_client_two == uid) {
        return tmp;
    } else {
        return NULL;
    }
}


void reset_rps_lobby(node **head){
    struct node *current = *head;
    current->RPSGame.unique_game_id_player_one = 0;
    current->RPSGame.unique_game_id_player_two = 0;
//    current->RPSGame.unique_game_id = 2;
    current->RPSGame.fd_client_player_two = 0;
    current->RPSGame.fd_client_player_one = 0;
}

void reset_ttt_lobby(node **head) {
    struct node *current = *head;
    current->TTTGame.client_x_uid = 2;
    current->TTTGame.client_o_uid = 2;
    current->TTTGame.client_x = 0;
    current->TTTGame.client_o = 0;
}

void delete_link(node **head, int32_t game_id){
    struct node *current = *head;
    struct node *prev = NULL;

    do {
        if(current->uid_client_two == game_id || current->uid_client_one == game_id) {
            break;
        }
        prev = current;
        current = current -> next;
    } while (current);

    if(current == *head) {
        prev = *head;
        *head = current -> next;
        free(prev);
        return;
    }
    if(current->next == NULL) {
        prev->next = NULL;
        free(current);
        return;
    }

    prev->next = current->next;
    free(current);
    return;
}


//void print_rps_games(node **head) {
//    struct node *current = *head;
//    while(current) {
//        if (current->RPSGame.unique_game_id > 0) {
//            printf(" --- > RPS GAME ID: %u, client one: %d, client two %d \n", current->game_id, current->RPSGame.fd_client_player_one, current->RPSGame.fd_client_player_two);
//        }
//         current = current->next;
//    }
//}

void print_ttt_collection(node **head) {
    struct node *current = *head;
    while(current) {
        printf(" --- > client one: %d, client two %d \n", current->TTTGame.client_x, current->TTTGame.client_o);
    current = current->next;
    }
}

void deinit(node **head){
    return;
};

//SingleTTTGameEnv * execute_ttt_turn(node **head, int32_t game_id_to_find, int8_t requested){
//    struct node *current = head;
//    struct node *tmp;
//
//    do{
//        tmp = current;
//        current = current ->next;
//    } while (current -> uid_client_one != game_id_to_find || current->uid_client_two != game_id_to_find);
//
//     return &current->TTTGame;
//
//}

//SingleRPSGameEnv * execute_rps_turn(node **head, int32_t game_id_to_find, int8_t requested) {
//    struct node *current = *head;
//    struct node *tmp;
//
//    do {
//        tmp = current;
//        current = current->next;
//    } while(current -> game_id != game_id_to_find);
//
//    // todo error check if game goes not exist
//    return &current->RPSGame;
//}

//int find_remaining_by_id(rps_node **head, int cfd);
//
//SingleRPSGameEnv * rematch(rps_node **head, int game_id);

//
// Created by emu on 2021-04-08.
//

#include "shared.h"
#include "fsm.h"

#ifndef A01_CLINT_EMERALD_RPS_LL_H
#define A01_CLINT_EMERALD_RPS_LL_H

typedef struct node   {
    SingleRPSGameEnv RPSGame;
    SingleTTTGameEnv TTTGame;
    uint8_t game_type;

    uint32_t uid_client_one;
    uint32_t uid_client_two;

    struct node *next;
} node;

int init_lobbies(node **head);

int insert_new_rps_game(node **head, SingleRPSGameEnv* new_env);
int insert_new_ttt_game(node **head, SingleTTTGameEnv* new_env);

void reset_rps_lobby(node **head);
void reset_ttt_lobby(node **head);

node * return_link_by_uid(node ** head, uint32_t uid);

void delete_link(node **head, int32_t game_id);

void print_rps_collection(node **head);
void print_ttt_collection(node **head);
void print_entire_collection(node **head);

void deinit(node **head);

SingleRPSGameEnv * execute_rps_turn(node **head, int32_t game_id_to_find, int8_t requested);
SingleTTTGameEnv * execute_ttt_turn(node **head, int32_t game_id_to_find, int8_t requested);


//int find_remaining_by_id(rps_node **head, int cfd);
//SingleRPSGameEnv * rematch(rps_node **head, int game_id);


#endif //A01_CLINT_EMERALD_RPS_LL_H

//
// Created by emu on 2021-03-03.
//
#include "shared.h"
#include "fsm.h"
#include "v2_FSM.h"


#ifndef A01_CLINT_EMERALD_GAME_COLLECTION_LINKED_LIST_H
#define A01_CLINT_EMERALD_GAME_COLLECTION_LINKED_LIST_H

typedef struct node {
    GameEnvironment game_env;
    int game_id;
    struct node *next;
} node;

int init(node **head);
int insert_new_game(node **head,  int game_id, int xfd, int ofd);
void reset_lobby(node **head);
void delete(node **head, int game_id);
void print(node **head);
void deinit(node **head);
int execute_turn(node **head, int game_id_to_find, int8_t requested);
int find_remaining_by_id(node **head, int cfd);
GameEnvironment * rematch(node **head, int game_id);

#endif //A01_CLINT_EMERALD_GAME_COLLECTION_LINKED_LIST_H

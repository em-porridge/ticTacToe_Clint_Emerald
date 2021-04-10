//
// Created by emu on 2021-04-07.
//

//
// Created by emu on 2021-03-03.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "v2_LinkedList_GameCollections.h"


int init(node **head)
{
    *head = malloc(sizeof(struct node));
    if (!*head) {
        fprintf(stderr, "Failed to init a linked list\n");
        return 1;
    }

    GameEnvironment lobby;
    lobby.game_ID = 2;
    lobby.fd_client_O = 0;
    lobby.fd_client_X = 0;

    (*head)->game_env = lobby;
    (*head)->game_id = 2;

    (*head)->next = NULL;
    return 0;
}


// sends game env to FSM returns game over
int execute_turn(node **head, int game_id_to_find, int8_t requested){
    struct node * current = *head;
    struct node * tmp;

    do {
        tmp = current;
        current = current->next;
    } while (current->game_id != game_id_to_find);

    current->game_env.byte_input = requested;
    mainaroo(&current->game_env);

    if(current->game_env.game_over == true) {
        return 1;
    }

    return 0;
}

// inserts a new game in to linked list
int insert_new_game(node **head, int game_id, int xfd, int ofd){
    struct node *current = *head;
    struct node *tmp;

    do {
        tmp = current;
        current = current->next;
    } while (current);

    /* create a new node after tmp */
    struct node *new = malloc(sizeof(struct node));
    if (!new) {
        fprintf(stderr, "Failed to insert a new element\n");
        return 1;
    }
    new->next = NULL;

    GameEnvironment new_env;
    new_env.game_ID = game_id;
    new_env.fd_client_O = ofd;
    new_env.fd_client_X = xfd;
    new_env.fd_current_player = xfd;
    new_env.current_player =  X;
    new_env.play_count = 0;
    new_env.game_over = false;

    for(int i = 0; i < 9; i++){
        new_env.game_board[i] = BLANK_SPACE;
    }

    new->game_id = game_id;
    new->game_env = new_env;
    tmp->next = new;
    return 0;
}

// resets a new game withing an existing node
GameEnvironment * rematch(node **head, int game_id){
    struct node *current = *head;
    struct node *tmp;

    while(current->next != NULL | current->game_id != game_id){
        tmp = current;
        current=current->next;
    }
    // reset board, game over, game count

    current->game_env.game_over = false;
    current->game_env.game_over = 0;

    for(int i = 0; i < 9; i++){
        current->game_env.game_board[i] = BLANK_SPACE;
    }

    return &current->game_env;
}

//todo break this down

// finds the remaining player left over from disconnect, modifies the lobby and send 1 if ready to start new game
int find_remaining_by_id(node **head, int cfd){
    struct node * lobs = *head;
    struct node * current = *head;
    struct node * tmp;
    uint8_t disconnect = DISCONNECT;
    int remaining_fd;

    // finds the node where the disconnection happens, saves remaining client fd, deletes node
    while(current->next){
        tmp = current;
        current = current->next;
        if(current->game_env.fd_client_O == cfd){
            remaining_fd = current->game_env.fd_client_X;
            send(remaining_fd, &disconnect, sizeof (disconnect), 0);
//            delete(head, current->game_id);
        }
        if(current->game_env.fd_client_X == cfd){
            remaining_fd = current->game_env.fd_client_O;
            send(remaining_fd, &disconnect, sizeof (disconnect), 0);
//            delete(head, current->game_id);
        }
    }

    if(remaining_fd < 1 ){
        printf("Error");
        return -1;
    } else if(lobs->game_id == 2){
        // both have disconnected so restart lobby
        reset_lobby(&lobs);
        return 0;
    } else {
        if(lobs->game_env.fd_client_X == 0){
            // have to wait for other players
            lobs->game_env.fd_client_X = remaining_fd;
            lobs->game_env.game_ID = tmp->game_id;
            return 0;
        } else {
            // lobby full ready to start new game
            lobs->game_env.fd_client_O = remaining_fd;
            return 1;
        }
    }
}

void delete(node **head,  int game_id)
{
    struct node *current = *head;
    struct node *prev = NULL;

    do {
        if (current->game_id == game_id) {
            break;
        }
        prev = current;
        current = current->next;
    } while (current);

    /* if the first element */
    if (current == *head) {
        /* reuse prev */
        prev = *head;
        *head = current->next;
        free(prev);
        return;
    }

    /* if the last element */
    if (current->next == NULL) {
        prev->next = NULL;
        free(current);
        return;
    }
    prev->next = current->next;
    free(current);
    return;
}


void print(node **head)
{
    struct node *current = *head;
    while (current) {
        printf("current data: %d, address: %p\n", current->game_id,
               current);
        current = current->next;
    }
}

void deinit(node **head)
{
    struct node *node = *head;
    do {
        struct node *tmp;
        tmp = node;
        node = node->next;
        free(tmp);
    } while (node);
}

// restarts lobby to default game id and client fd
void reset_lobby(node **head){

    struct node *current = *head;
    current->game_env.game_ID = 2;
    current->game_env.fd_client_O = 0;
    current->game_env.fd_client_X = 0;
    current->game_id= 2;

}



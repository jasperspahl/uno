//
// Created by jasper on 10/14/23.
//

#ifndef UNO_CLIENT_H
#define UNO_CLIENT_H

#include <stddef.h>
#include <stdbool.h>
#include "types.h"

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    size_t cards_length;
} player;

typedef struct {
    player* players;
    size_t player_count;
    card current_card;
    int own_id;
    hand own_hand;
    char own_name[MAX_NAME_LENGTH];
    bool started;
    int socket;
} game_state;

void print_game_state(game_state* game_state);

player * get_player_by_id(game_state* game_state, int id);

bool handle_action(game_state *game_state);

bool handle_play( game_state *game_state);
bool handle_draw( game_state *game_state);
bool handle_pass( game_state *game_state);
bool handle_say_uno( game_state *game_state);
bool handle_set_player_data( game_state *game_state);
bool handle_start_game( game_state *game_state);
bool handle_player_joined( game_state *game_state);
bool handle_your_turn( game_state *game_state);


#endif //UNO_CLIENT_H

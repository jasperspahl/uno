#ifndef UNO_SERVER_H
#define UNO_SERVER_H

#include<stddef.h>
#include<stdint.h>
#include<pthread.h>
#include"types.h"


typedef struct s_deck {
    card cards[CARDS_IN_DECK];
} deck;


typedef struct s_player {
    hand hand;
    struct s_player* next;
    struct s_player* prev;
    char name[MAX_NAME_LENGTH];
    int id;
    int socket;
    pthread_t thread;
} player;

typedef struct s_game_state {
    direction direction;
    player* current_player;
    deck deck;
    card* discard_pile;
    size_t discard_pile_length;
    card* draw_pile;
    size_t draw_pile_length;
    bool started;
} game_state;

void init_deck(deck* deck);
void shuffle_deck(deck* deck);
void init_game_state();
void deal_cards();
size_t count_players();
void start_game();
void end_game();

card get_next_draw_card();


bool handle_action(player* player);

bool handle_play(player* player);
bool handle_draw(player* player);
bool handle_pass(player* player);
bool handle_say_uno(player* player);
bool handle_set_player_name(player* player);

void send_card_to_player(player* player, card card);
void send_player_joined(player* p);
void send_player_name(player* player);
void send_card_played(card card);

void print_hand(hand hand);

void print_game_state(game_state* game_state);
#endif //UNO_SERVER_H

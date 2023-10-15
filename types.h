//
// Created by jasper on 10/14/23.
//

#ifndef UNO_TYPES_H
#define UNO_TYPES_H

#define MAX_PLAYERS 10
#define MAX_NAME_LENGTH 20
#define CARDS_PER_PLAYER 7
#define CARDS_IN_DECK 108
#define PORT 8666

typedef enum {
    RED,
    BLUE,
    GREEN,
    YELLOW,
    BLACK
} card_color;

typedef enum {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    SKIP,
    REVERSE,
    DRAW_TWO,
    WILD,
    WILD_DRAW_FOUR
} card_value;

typedef enum {
    CLOCKWISE,
    COUNTER_CLOCKWISE
} direction;

typedef struct s_card {
    card_value value;
    card_color color;
} card;

typedef struct s_hand {
    card* cards;
    size_t length;
} hand;

typedef enum {
    PLAY,
    DRAW,
    PASS,
    SAY_UNO,
    SET_NAME,
    SET_PLAYER_DATA,
    START_GAME,
    PLAYER_JOINED,
    YOUR_TURN,
    GAME_OVER,
} action;

typedef enum {
    PLAYER_NAME,
    PLAYER_HAND_SIZE,
} player_data;

typedef struct {
    action action;
    char name[MAX_NAME_LENGTH];
} set_name_action;

typedef struct {
    action action;
    char card;
} card_action;

typedef struct {
    action action;
    player_data player_data;
    int id;
    char size;
} hand_size_action;

typedef struct {
    action action;
    player_data player_data;
    int id;
    char name[MAX_NAME_LENGTH];
} player_name_action;

typedef struct {
    action action;
    int id;
    char name[MAX_NAME_LENGTH];
} player_joined_action;

#endif //UNO_TYPES_H

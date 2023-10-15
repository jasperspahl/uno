#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "server.h"
#include "common.h"

pthread_mutex_t mutex;
game_state game;

void* handle_client(void *arg) {
    printf("New client connected\n");
    player* p = arg;
    action action;
    ssize_t str_len;

    pthread_mutex_lock(&mutex);
    check(send(p->socket, &p->id, sizeof(p->id), 0));

    check(str_len = read(p->socket, &action, sizeof(action)));
    if (str_len == 0) {
        printf("Player %d disconnected\n", p->id);
        close(p->socket);
        return NULL;
    }
    assert(action == SET_NAME);
    char name[MAX_NAME_LENGTH];
    check(recv(p->socket, name, sizeof(name), 0));
    strcpy(p->name, name);
    send_player_joined(p);
    player * cur = p->next;
    while (cur != p) {
        player_joined_action joined_action = {PLAYER_JOINED, cur->id};
        strcpy(joined_action.name, cur->name);
        check(send(p->socket, &joined_action, sizeof(joined_action), 0));
        cur = cur->next;
    }
    pthread_mutex_unlock(&mutex);

    while (handle_action(p));

    close(p->socket);
    return NULL;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    int server_sock;

    struct sockaddr_in serv_addr;

    check(server_sock = socket(PF_INET, SOCK_STREAM, 0));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    pthread_mutex_init(&mutex, NULL);
    memset(&game, 0, sizeof(game));
    init_game_state();

    check(bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)));
    check(listen(server_sock, 5));
    printf("Server listening on port %d\n", PORT);

    do {
        int client_sock;
        check(client_sock = accept(server_sock, NULL, NULL));
        pthread_mutex_lock(&mutex);
        if (game.started == true) {
            printf("Game already started\n");
            close(client_sock);
            pthread_mutex_unlock(&mutex);
            break;
        }
        // Add player to linked list and set as current player
        if (game.current_player == NULL) {
            game.current_player = malloc(sizeof(player));
            game.current_player->next = game.current_player;
            game.current_player->prev = game.current_player;
        }
        else {
            player *new_player = malloc(sizeof(player));
            new_player->next = game.current_player;
            new_player->prev = game.current_player->prev;
            game.current_player->prev->next = new_player;
            game.current_player->prev = new_player;
            game.current_player = new_player;
        }
        game.current_player->socket = client_sock;
        game.current_player->hand.cards = NULL;
        game.current_player->hand.length = 0;
        game.current_player->id = (int) count_players();
        pthread_mutex_unlock(&mutex);

        pthread_create(&game.current_player->thread, NULL, handle_client, game.current_player);
        pthread_mutex_lock(&mutex);
        pthread_mutex_unlock(&mutex);
    } while (count_players() < MAX_PLAYERS);

    player* cursor = game.current_player;
    do {
        pthread_join(cursor->thread, NULL);
        cursor = cursor->next;
    } while (cursor != game.current_player);
    close(server_sock);
    return 0;
}


void init_deck(deck *deck) {
    int i = 0;
    for (card_color color = RED; color <= YELLOW; color++) {
        // Add two of each card, except for zero
        for (int j = 0; j < 2; j++) {
            for (card_value value = ONE; value <= DRAW_TWO; value++) {
                deck->cards[i].color = color;
                deck->cards[i].value = value;
                i++;
            }
        }
        deck->cards[i].color = color;
        deck->cards[i].value = ZERO;
        i++;
    }
    // Add four wild cards
    for (int j = 0; j < 4; j++) {
        deck->cards[i].color = BLACK;
        deck->cards[i].value = WILD;
        i++;
    }
    // Add four wild draw four cards
    for (int j = 0; j < 4; j++) {
        deck->cards[i].color = BLACK;
        deck->cards[i].value = WILD_DRAW_FOUR;
        i++;
    }
    assert(i == 108);
}

void shuffle_deck(deck *deck) {
    for (size_t i = sizeof(deck->cards) / sizeof(card) - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        card temp = deck->cards[j];
        deck->cards[j] = deck->cards[i];
        deck->cards[i] = temp;
    }
}

void init_game_state() {
    game.direction = CLOCKWISE;
    game.current_player = NULL;
    game.discard_pile = NULL;
    game.discard_pile_length = 0;
    game.draw_pile = NULL;
    game.draw_pile_length = 0;
    init_deck(&game.deck);
    shuffle_deck(&game.deck);
    // initialize draw pile
    game.draw_pile = malloc(sizeof(card) * CARDS_IN_DECK);
    game.draw_pile_length = 108;
    memcpy(game.draw_pile, game.deck.cards, sizeof(game.deck.cards));
    // initialize discard pile
    game.discard_pile = malloc(sizeof(card) * CARDS_IN_DECK);
    game.discard_pile_length = 0;
}

void deal_cards() {
    for (size_t i = 0; i < CARDS_PER_PLAYER; i++) {
        player *current_player = game.current_player;
        do {
            card card = get_next_draw_card();
            current_player->hand.cards = realloc(current_player->hand.cards, sizeof(card) * (current_player->hand.length + 1));
            current_player->hand.cards[current_player->hand.length] = card;
            current_player->hand.length++;
            send_card_to_player(current_player, card);
            current_player = current_player->next;
        } while (current_player != game.current_player);
    }
    card card = get_next_draw_card();
    game.discard_pile[game.discard_pile_length] = card;
    game.discard_pile_length++;
    send_card_played(card);
}


void print_hand(hand hand) {
    for (size_t i = 0; i < hand.length; i++) {
        print_card(hand.cards[i]);
        printf(" ");
    }
    printf("\n");
}

void print_game_state(game_state *game_state) {
    printf("Direction: %s\n", game_state->direction == CLOCKWISE ? "clockwise" : "counterclockwise");
    if (*game_state->current_player->name == '\0') {
        printf("Current player: Player %d\n", game_state->current_player->id);
    } else {
        printf("Current player: %s\n", game_state->current_player->name);
    }
    printf("Discard pile: ");
    if (game_state->discard_pile_length == 0) {
        printf("empty\n");
    } else {
        print_card(game_state->discard_pile[game_state->discard_pile_length - 1]);
        printf("\n");
    }
    printf("\n");
    printf("Draw pile: %zu\n", game_state->draw_pile_length);
    printf("Players:\n");
    player *current_player = game_state->current_player;
    do {
        printf("  Player %d: ", current_player->id);
        print_hand(current_player->hand);
        current_player = current_player->next;
    } while (current_player != game_state->current_player);
}

void start_game() {
    size_t num_players = count_players();
    if (num_players < 2) {
        printf("Not enough players to start game\n");
        return;
    }
    if (game.started) {
        printf("Game already started\n");
        return;
    }
    game.started = true;
    deal_cards();
    game.discard_pile[game.discard_pile_length] = get_next_draw_card();
    game.discard_pile_length++;
    action a = YOUR_TURN;
    check(send(game.current_player->socket, &a, sizeof(a), 0));
    print_game_state(&game);
}

size_t count_players() {
    size_t count = 0;
    player* current_player = game.current_player;
    player* cursor = game.current_player;
    do {
        count++;
        cursor = cursor->next;
    } while (cursor != current_player);
    return count;
}

card get_next_draw_card() {
    card card = game.draw_pile[game.draw_pile_length - 1];
    game.draw_pile_length--;
    return card;
}

void send_card_to_player(player *p, card card) {
    card_action action = {DRAW, card_to_char(card)};
    send(p->socket, &action, sizeof(action), 0);
    player * cur = p->next;
    hand_size_action hand_size_action = {SET_PLAYER_DATA, PLAYER_HAND_SIZE, p->id, (char)p->hand.length};
    while (cur != p) {
        send(cur->socket, &hand_size_action, sizeof(hand_size_action), 0);
        cur = cur->next;
    }
}

bool handle_set_player_name(player *player) {
    char name[MAX_NAME_LENGTH];
    recv(player->socket, name, sizeof(name), 0);
    strcpy(player->name, name);
    printf("Player %d set name to %s\n", player->id, player->name);
    send_player_name(player);
    return true;
}

void send_player_name(player *p) {
    player_name_action action = {SET_PLAYER_DATA, PLAYER_NAME, p->id};
    strcpy(action.name, p->name);
    player *cur = p->next;
    while (cur != p) {
        send(cur->socket, &action, sizeof(action), 0);
        cur = cur->next;
    }
}

bool handle_play(player *player) {
    // TODO: check if player has card
    return true;
}

bool handle_draw(player *player) {
    if (player != game.current_player)
        return true;

    if (game.draw_pile_length == 0)
        check(-1 /* TODO: shuffle discard pile into draw pile*/);

    card card = get_next_draw_card();
    player->hand.cards = realloc(player->hand.cards, sizeof(card) * (player->hand.length + 1));
    player->hand.cards[player->hand.length] = card;
    player->hand.length++;
    send_card_to_player(player, card);
    return true;
}

bool handle_pass(player *player) {
    return true;
}

bool handle_say_uno(player *player) {
    return true;
}

void send_player_joined(player *p) {
    printf("Player %d %s joined\n", p->id, p->name);
    player_joined_action action = {PLAYER_JOINED, p->id};
    strcpy(action.name, p->name);
    player * cur = p->next;
    while (cur != p) {
        send(cur->socket, &action, sizeof(action), 0);
        cur = cur->next;
    }
}

void send_card_played(card card) {
    card_action action = {PLAY, card_to_char(card)};
    player * cur = game.current_player;
    do {
        send(cur->socket, &action, sizeof(action), 0);
        cur = cur->next;
    }
    while (cur != game.current_player);
}

bool handle_action(player *p) {
    action action;
    size_t str_len;
    check(str_len = read(p->socket, &action, sizeof(action)));
    if (str_len == 0) {
        printf("Player %d disconnected\n", p->id);
        // TODO: remove player from list
        return false;
    }
    bool result = true;
    pthread_mutex_lock(&mutex);
    printf("Received action %s from player %d\n", action_to_string(action), p->id);
    switch (action)
    {
        case PLAY:
            result = handle_play(p);
            break;
        case DRAW:
            result = handle_draw(p);
            break;
        case PASS:
            result = handle_pass(p);
            break;
        case SAY_UNO:
            result = handle_say_uno(p);
            break;
        case SET_NAME:
            result =  handle_set_player_name(p);
            break;
        case START_GAME:
            start_game();
            result = true;
            break;
        case SET_PLAYER_DATA:
        case PLAYER_JOINED:
        case YOUR_TURN:
        case GAME_OVER:
            printf("Invalid Request from Player %d: %s", p->id, p->name);
            result = false;
            break;
    }
    pthread_mutex_unlock(&mutex);
    return result;
}

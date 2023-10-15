#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"
#include "common.h"

pthread_t start_game_thread;

void* start_game_when_enter_is_pressed(void *arg) {
    game_state *game = (game_state *) arg;
    while (1) {
        int c = getchar();
        if (c == '\n' && game->player_count) {
            action action = START_GAME;
            check(send(game->socket, &action, sizeof(action), 0));
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    game_state game;
    memset(&game, 0, sizeof(game_state));
    if (argc != 3) {
        printf("Usage : %s <IP> <name>\n", argv[0]);
        exit(1);
    }
    if (strlen(argv[2]) >= MAX_NAME_LENGTH - 1) {
        printf("Name too long\n");
        exit(1);
    }
    strcpy(game.own_name, argv[2]);


    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int s = getaddrinfo(argv[1], "8666", &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        game.socket = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (game.socket == -1)
            continue;

        if (connect(game.socket, rp->ai_addr, rp->ai_addrlen) != -1)
            break;		   /* Success */

        close(game.socket);
    }

    freeaddrinfo(result);	   /* No longer needed */

    if (rp == NULL) {		   /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    ssize_t content_len;

    check(content_len = recv(game.socket, &game.own_id, sizeof(game.own_id), 0));

    set_name_action set_name;
    set_name.action = SET_NAME;
    strcpy(set_name.name, game.own_name);
    check(send(game.socket, &set_name, sizeof(set_name), 0));

    pthread_create(&start_game_thread, NULL, start_game_when_enter_is_pressed, &game);

    while (handle_action(&game))
    {
        print_game_state(&game);
    }
    close(game.socket);
    return 0;
}

bool handle_player_joined( game_state *game_state) {
    printf("Player joined\n");
    game_state->players = realloc(game_state->players, sizeof(player) * (game_state->player_count + 1));
    memset(&game_state->players[game_state->player_count], 0, sizeof(player));
    check(recv(game_state->socket, &game_state->players[game_state->player_count].id, sizeof(int), 0));
    check(recv(game_state->socket, &game_state->players[game_state->player_count].name, sizeof(char) * MAX_NAME_LENGTH, 0));
    game_state->player_count++;
    printf("\t %s joined\n", game_state->players[game_state->player_count - 1].name);
    return true;
}

void print_game_state(game_state *game_state) {
    printf("Players:\n");
    for (int i = 0; i < game_state->player_count; ++i) {
        printf("%s: %zu Cards\n", game_state->players[i].name, game_state->players[i].cards_length);
    }
    printf("Cards:\n");
    print_cards(game_state->own_hand.cards, game_state->own_hand.length);
}

bool handle_set_player_data(game_state *game_state) {
    player_data pd;
    check(recv(game_state->socket, &pd, sizeof(player_data), 0));
    int pid;
    check(recv(game_state->socket, &pid, sizeof(int), 0));
    char name[MAX_NAME_LENGTH];
    char hand_size;
    switch (pd) {
        case PLAYER_NAME:
            check(recv(game_state->socket, &name, sizeof(char) * MAX_NAME_LENGTH, 0));
            break;
        case PLAYER_HAND_SIZE:
            check(recv(game_state->socket, &hand_size, sizeof(char), 0));
            break;
    }
    player *p = get_player_by_id(game_state, pid);
    if (p == NULL) {
        printf("Player not found\n");
        exit(1);
    }
    switch (pd) {
        case PLAYER_NAME:
            strcpy(p->name, name);
            break;
        case PLAYER_HAND_SIZE:
            p->cards_length = (size_t)hand_size;
            break;
    }
    return true;
}

player *get_player_by_id(game_state *game_state, int id) {
    for (int i = 0; i < game_state->player_count; ++i) {
        if (game_state->players[i].id == id) {
            return &game_state->players[i];
        }
    }
    return NULL;
}

bool handle_your_turn(game_state *game_state) {
    printf("It's your turn\n");
    return true;
}

bool handle_play(game_state *game_state) {
    printf("Received play\n");
    char card_char;
    check(recv(game_state->socket, &card_char, sizeof(char), 0));
    game_state->current_card = char_to_card(card_char);
    return true;
}

bool handle_draw(game_state *game_state) {
    printf("Received draw\n");
    char card_char;
    check(recv(game_state->socket, &card_char, sizeof(char), 0));
    card card = char_to_card(card_char);
    game_state->own_hand.cards = realloc(game_state->own_hand.cards, sizeof(card) * (game_state->own_hand.length + 1));
    game_state->own_hand.cards[game_state->own_hand.length] = card;
    game_state->own_hand.length++;

    return true;
}

bool handle_pass(game_state *game_state) {
    printf("Received pass\n");

    return true;
}

bool handle_say_uno(game_state *game_state) {
    printf("Received say uno\n");

    return true;
}

bool handle_start_game(game_state *game_state) {
    printf("Game started\n");
    game_state->started = true;
    return true;
}

bool handle_action(game_state *game_state) {
    action action;
    size_t content_len;
    check(content_len = recv(game_state->socket, &action, sizeof(action), 0));
    if (content_len == 0) {
        printf("Server closed connection\n");
        return false;
    }
    bool result = true;
    printf("Received action: %s\n", action_to_string(action));
    switch (action)
    {
        case PLAY:
            result = handle_play(game_state);
            break;
        case DRAW:
            result = handle_draw(game_state);
            break;
        case PASS:
            result = handle_pass(game_state);
            break;
        case SAY_UNO:
            result = handle_say_uno(game_state);
            break;
        case SET_PLAYER_DATA:
            result = handle_set_player_data(game_state);
            break;
        case START_GAME:
            pthread_cancel(start_game_thread);
            result = handle_start_game(game_state);
            break;
        case PLAYER_JOINED:
            result = handle_player_joined(game_state);
            break;
        case YOUR_TURN:
            result = handle_your_turn(game_state);
            break;
        case SET_NAME:
        case GAME_OVER:
            return false;
    }

    return result;
}

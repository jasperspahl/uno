//
// Created by jasper on 10/14/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"

char card_to_char(card card) {
    return (char) (card.value | (card.color << 4));
}

card char_to_card(char c) {
    card result = {0};
    result.value = c & 0x0F;
    result.color = (card_color) (c >> 4);
    return result;
}
void print_card(card card) {
    switch (card.color) {
        case RED:
            printf("\033[30;101m");
            break;
        case BLUE:
            printf("\033[30;104m");
            break;
        case GREEN:
            printf("\033[30;102m");
            break;
        case YELLOW:
            printf("\033[30;103m");
            break;
        case BLACK:
            printf("\033[0;40m");
            break;
    }
    if (card.value <= NINE) {
        printf("  %d ", card.value);
    } else {
        switch (card.value) {
            case SKIP:
                printf("Skip");
                break;
            case REVERSE:
                printf("Revs");
                break;
            case DRAW_TWO:
                printf(" +2 ");
                break;
            case WILD:
                printf("Wild");
                break;
            case WILD_DRAW_FOUR:
                printf(" +4 ");
                break;
            default:
                printf("This is not allowed to happen\n");
                exit(1);
        }
    }
    printf("\033[0m");
}

void print_cards(card *cards, size_t length) {
    for (size_t i = 0; i < length; i++) {
        print_card(cards[i]);
        printf(" ");
    }
    printf("\n");
}

void error_handling(const char *fmt, ...) {
    int errno_save;
    va_list ap;

    errno_save = errno;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);

    if (errno_save != 0) {
        fprintf(stderr, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    exit(1);
}

char *action_to_string(action action) {
    switch (action) {
        case PLAY:
            return "PLAY";
        case DRAW:
            return "DRAW";
        case PASS:
            return "PASS";
        case SAY_UNO:
            return "SAY_UNO";
        case SET_NAME:
            return "SET_NAME";
        case SET_PLAYER_DATA:
            return "SET_PLAYER_DATA";
        case START_GAME:
            return "START_GAME";
        case PLAYER_JOINED:
            return "PLAYER_JOINED";
        case YOUR_TURN:
            return "YOUR_TURN";
        case GAME_OVER:
            return "GAME_OVER";
    }
}

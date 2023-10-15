//
// Created by jasper on 10/14/23.
//

#ifndef UNO_COMMON_H
#define UNO_COMMON_H

#include <stddef.h>
#include "types.h"

#define check(x) if ((x)==-1) error_handling(#x " failed at " __FILE__ ":%d", __LINE__)

void error_handling(const char *fmt,...);

char card_to_char(card card);
card char_to_card(char c);

void print_card(card card);
void print_cards(card* cards, size_t length);

char* action_to_string(action action);

#endif //UNO_COMMON_H

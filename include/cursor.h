#ifndef CURSOR_H
#define CURSOR_H

#include "common.h"

void move_cursor(int x, int y);
void hide_cursor();
void show_cursor();
void get_cursor(int *x, int *y);


#endif // CURSOR_H
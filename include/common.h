#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>
#include <conio.h>

#define MAX_LINES 1000
#define MAX_COLUMNS 400

typedef enum _cursor { 
    cur_left,
    cur_right,
    cur_up,
    cur_down,
    cur_enter,
    cur_backspace
} CURSOR;

typedef enum _commands {
    cmd_save,
    cmd_exit,
    cmd_open,
    cmd_new
} COMMANDS;

typedef enum _state {
    normal,
    insert
} STATE;

typedef enum _save_open_mode {
    save,
    open
} SAVE_OPEN_MODE;

#endif // COMMON_H
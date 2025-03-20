#ifndef DEFINES_H
#define DEFINES_H

#define MAX_LINES 1000
#define MAX_COLUMNS 400
#define MAX_LINE_LENGTH 1000
#define TAB_SIZE 4       // Number of spaces for a tab

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
    insert,
    command
} STATE;

typedef enum _save_open_mode {
    save,
    open
} SAVE_OPEN_MODE;

typedef enum _keys {
    key_up = 72,
    key_down = 80,
    key_left = 75,
    key_right = 77,
    key_enter = 13,
    key_esc = 27,
    key_colon = 58,
} KEYS; 

#endif // DEFINES_H
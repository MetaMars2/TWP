#ifndef EDITOR_H
#define EDITOR_H

#include "common.h"

#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004

typedef struct _editor {
    char lines[MAX_LINES][MAX_COLUMNS];
    int line_count;
    int cursor_x; 
    int cursor_y;
    int scroll_offset;  // Track which line is at the top of the screen
    STATE state;
    char filename[FILENAME_MAX];
    char filepath[FILENAME_MAX];
    bool is_saved;
    HANDLE current_buffer;
    HANDLE new_buffer;
    char command_buffer[MAX_COLUMNS];
    int command_position;
} EDITOR;


void init_editor(EDITOR* editor);

#endif // EDITOR_H
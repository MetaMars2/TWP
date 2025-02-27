#ifndef EDITOR_H
#define EDITOR_H

#include "common.h"

typedef struct _editor {
    char lines[MAX_LINES][MAX_COLUMNS];
    int line_count;
    int cursor_x; 
    int cursor_y; 
    STATE state;
    char filename[FILENAME_MAX];
    char filepath[FILENAME_MAX];
    bool is_saved;
    HANDLE current_buffer;
    HANDLE new_buffer;
} EDITOR;

void init_editor(EDITOR* editor);

#endif // EDITOR_H
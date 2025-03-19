#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"
#include "editor.h"

void render_editor(EDITOR* editor);
void set_cursor_position(EDITOR* editor, int x, int y);
void print_command_prompt(EDITOR* editor, DWORD dw_bytes_written);
void print_status(EDITOR* editor, int cur_x, int cur_y, DWORD dw_bytes_written);

#endif // DISPLAY_H
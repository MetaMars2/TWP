#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"
#include "editor.h"

void render_editor(EDITOR* editor);
void set_cursor_position(EDITOR* editor, int x, int y);

#endif // DISPLAY_H
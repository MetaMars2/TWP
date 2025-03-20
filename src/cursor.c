#include "../include/cursor.h"
#include "../include/editor.h"

void set_cursor_position(EDITOR* editor, int x, int y) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
    int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;

    // Validate Y position
    if (y >= editor->line_count) {
        y = editor->line_count - 1;
    }
    if (y < 0) {
        y = 0;
    }
    
    // Handle scrolling when cursor moves off-screen
    if (y < editor->scroll_offset) {
        editor->scroll_offset = y;
    }
    else if (y >= editor->scroll_offset + screen_height) {
        editor->scroll_offset = y - screen_height + 1;
    }
    
    // Validate X position based on current line length
    int line_length = strlen(editor->lines[y]);
    if (x > line_length) {
        x = line_length;
    }
    if (x < 0) {
        x = 0;
    }
    
    editor->cursor_x = x;
    editor->cursor_y = y;
    
    // Update cursor position on screen
    int screen_y = y - editor->scroll_offset;
    SetConsoleCursorPosition(editor->current_buffer, (COORD){x, screen_y});
}
#include "../include/display.h"

void render_editor(EDITOR* editor) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);

    int cur_x = editor->cursor_x;
    int cur_y = editor->cursor_y - editor->scroll_offset;

    DWORD dw_bytes_written = 0;
    COORD cursor_pos = {0, 0};

    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(editor->new_buffer, ' ', length, cursor_pos, &dw_bytes_written);
    FillConsoleOutputAttribute(editor->new_buffer, csbi.wAttributes, length, cursor_pos, &dw_bytes_written);

    // Calculate visible screen area
    int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1; // -1 for status bar
    
    // Render only the visible lines
    for(int i = 0; i < screen_height && i + editor->scroll_offset < editor->line_count; i++) {
        cursor_pos.Y = i;
        cursor_pos.X = 0;
        WriteConsoleOutputCharacter(editor->new_buffer, editor->lines[i + editor->scroll_offset], 
                                   strlen(editor->lines[i + editor->scroll_offset]), cursor_pos, &dw_bytes_written);
    }
    
    int last_line = csbi.srWindow.Bottom;
    COORD status_pos = {0, last_line};
    
    char status_text[256];
    sprintf(status_text, " %-10s %s [Line %d/%d]",
            editor->state == normal ? "NORMAL" : "INSERT", 
            editor->filename,
            editor->cursor_y + 1,  // 1-based line number
            editor->line_count);
    
    WriteConsoleOutputCharacter(editor->new_buffer, status_text, 
                              strlen(status_text), status_pos, &dw_bytes_written);

    HANDLE temp = editor->current_buffer;
    editor->current_buffer = editor->new_buffer;
    editor->new_buffer = temp;

    SetConsoleActiveScreenBuffer(editor->current_buffer);
    SetConsoleCursorPosition(editor->current_buffer, (COORD){cur_x, cur_y});
}
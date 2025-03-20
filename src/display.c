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

    if(editor->state == normal || editor->state == insert) {
        print_status(editor, cur_x, cur_y, dw_bytes_written);
    } else {
        print_command_prompt(editor, dw_bytes_written);
    }

    HANDLE temp = editor->current_buffer;
    editor->current_buffer = editor->new_buffer;
    editor->new_buffer = temp;

    SetConsoleActiveScreenBuffer(editor->current_buffer);
    SetConsoleCursorPosition(editor->current_buffer, (COORD){cur_x, cur_y});
}

void print_status(EDITOR* editor, int cur_x, int cur_y, DWORD dw_bytes_written) {

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
    int last_line = csbi.srWindow.Bottom;
    COORD status_pos = {0, last_line};
    
    if(editor->state != command){
        char status[MAX_COLUMNS];
        sprintf(status, "%s | Line: %d | Column: %d | %s", 
            editor->state == normal ? "NORMAL" : "INSERT", 
            editor->cursor_y + 1, cur_x + 1,
            editor->is_saved ? "Saved" : "Unsaved");
        WriteConsoleOutputCharacter(editor->new_buffer, status, strlen(status), status_pos, &dw_bytes_written);
    } 
}

void print_command_prompt(EDITOR* editor, DWORD dw_bytes_written) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
    int last_line = csbi.srWindow.Bottom;
    COORD status_pos = {0, last_line};
    
    // Clear the line first
    FillConsoleOutputCharacter(editor->new_buffer, ' ', csbi.dwSize.X, status_pos, &dw_bytes_written);
    
    // Calculate how much of the command buffer we can display
    size_t max_cmd_len = MAX_COLUMNS - 2;  // -1 for colon, -1 for null terminator
    size_t cmd_len = strlen(editor->command_buffer);
    if (cmd_len > max_cmd_len) {
        cmd_len = max_cmd_len;
    }
    
    // Display the prompt and command text
    char display_text[MAX_COLUMNS];
    snprintf(display_text, MAX_COLUMNS - 1, ":%.*s", (int)cmd_len, editor->command_buffer);
    
    WriteConsoleOutputCharacter(editor->new_buffer, display_text, strlen(display_text), 
                              status_pos, &dw_bytes_written);
    
    // Position cursor at the end of the displayed command text
    SetConsoleCursorPosition(editor->current_buffer, 
                           (COORD){1 + cmd_len, last_line});
}
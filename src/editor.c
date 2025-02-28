#include "../include/editor.h"

void init_editor(EDITOR* editor) {
    memset(editor->lines, 0, sizeof(editor->lines));
    editor->lines[0][0] = '\0';
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->scroll_offset = 0;  // Initialize scroll position
    editor->state = normal;
    
    if (editor->filepath[0] == '\0') {
        strcpy(editor->filepath, "untitled.txt");
        strcpy(editor->filename, "untitled.txt");
    } else {
        char* filename_part = strrchr(editor->filepath, '\\');
        if (filename_part) {
            strcpy(editor->filename, filename_part + 1);
        } else {
            strcpy(editor->filename, editor->filepath);
        }
    }
    
    FILE* file = fopen(editor->filepath, "r");
    if (file) {
        char file_contents[MAX_LINES][MAX_COLUMNS];
        int line_count = 0;
        
        while (line_count < MAX_LINES && fgets(file_contents[line_count], MAX_COLUMNS, file)) {
            size_t len = strlen(file_contents[line_count]);
            if (len > 0 && file_contents[line_count][len-1] == '\n') {
                file_contents[line_count][len-1] = '\0';
            }
            line_count++;
        }
        fclose(file);
        
        if (editor->line_count > 1 || strlen(editor->lines[0]) > 0) {
            editor->is_saved = false;
            
            if (line_count == editor->line_count) {
                editor->is_saved = true;
                for (int i = 0; i < line_count; i++) {
                    if (strcmp(editor->lines[i], file_contents[i]) != 0) {
                        editor->is_saved = false;
                        break;
                    }
                }
            }
        } else {
            editor->is_saved = true;
        }
    } else {
        editor->is_saved = (editor->line_count == 1 && editor->lines[0][0] == '\0');
    }

    editor->current_buffer = GetStdHandle(STD_OUTPUT_HANDLE);
    editor->new_buffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
    
    DWORD mode;
    GetConsoleMode(editor->current_buffer, &mode);
    SetConsoleMode(editor->current_buffer, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleMode(editor->new_buffer, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    CONSOLE_CURSOR_INFO cursor_info = {100, TRUE};
    SetConsoleCursorInfo(editor->current_buffer, &cursor_info);
    SetConsoleCursorInfo(editor->new_buffer, &cursor_info);
}

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
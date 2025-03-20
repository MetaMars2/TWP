#include "../include/editor.h"

void init_editor(EDITOR* editor) {
    memset(editor->lines, 0, sizeof(editor->lines));
    editor->lines[0][0] = '\0';
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->scroll_offset = 0;  // Initialize scroll position
    editor->state = normal;
    editor->command_buffer[0] = '\0';
    editor->command_position = 0;
    
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


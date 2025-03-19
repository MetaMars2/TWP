#include "../include/input.h"
#include "../include/display.h"
#include "../include/fileops.h"

bool process_input(EDITOR* editor) {
    bool input_changed = false;

    if(_kbhit()) {
        int ch = _getch();
        input_changed = true;

        switch(editor->state){
            case normal:
                input_changed = state_normal_input(editor, ch, input_changed);
                break;
            case command:
                state_command_input(editor, ch, input_changed);
                break;
            case insert:
                state_insert_input(editor, ch, input_changed);
                break;
            
        }
    }

    return input_changed;
}

bool state_normal_input(EDITOR *editor, char ch, bool input_changed) {
    
    // State transitions
    if(ch == 'i'){
        editor->state = insert;
    } else if(ch == ':') {
        editor->state = command;
    }

    if(ch == 'h' && editor->cursor_x > 0) {
        set_cursor_position(editor, editor->cursor_x - 1, editor->cursor_y);
    } else if(ch == 'l') {
        size_t line_length = strlen(editor->lines[editor->cursor_y]);
        if(editor->cursor_x < line_length) {
            set_cursor_position(editor, editor->cursor_x + 1, editor->cursor_y);
        }
    } else if(ch == 'k' && editor->cursor_y > 0) {
        set_cursor_position(editor, editor->cursor_x, editor->cursor_y - 1);
    } else if(ch == 'j' && editor->cursor_y < editor->line_count - 1) {
        set_cursor_position(editor, editor->cursor_x, editor->cursor_y + 1);
    } 
    
    if(ch == 'u') {  // Page Up
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
        int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
        set_cursor_position(editor, editor->cursor_x, editor->cursor_y - screen_height);
    } else if(ch == 'd') {  // Page Down
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
        int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
        set_cursor_position(editor, editor->cursor_x, editor->cursor_y + screen_height);
    } 
    
    if(ch == 'g') {  // Go to top
        set_cursor_position(editor, 0, 0);
    } else if(ch == 'G') {  // Go to bottom
        set_cursor_position(editor, 0, editor->line_count - 1);
    }

    return input_changed;
}

void state_insert_input(EDITOR *editor, char ch, bool input_changed) {
    if(ch == 27) {
        editor->state = normal;
    } else if(ch == 0 || ch == 224) {
        ch = _getch();
        
        switch(ch) {
            case 72: // Up arrow
                if(editor->cursor_y > 0) {
                    set_cursor_position(editor, editor->cursor_x, editor->cursor_y - 1);
                }
                break;
                
            case 80: // Down arrow
                if(editor->cursor_y < editor->line_count - 1) {
                    set_cursor_position(editor, editor->cursor_x, editor->cursor_y + 1);
                }
                break;
                
            case 75: // Left arrow
                if(editor->cursor_x > 0) {
                    set_cursor_position(editor, editor->cursor_x - 1, editor->cursor_y);
                }
                break;
                
            case 77: // Right arrow
                if(editor->cursor_x < strlen(editor->lines[editor->cursor_y])) {
                    set_cursor_position(editor, editor->cursor_x + 1, editor->cursor_y);
                }
                break;
        }
    } else if(ch == 13) {
        int cur_line = editor->cursor_y;
        int cur_col = editor->cursor_x;

        if(editor->line_count < MAX_LINES) {
            for(int i = editor->line_count; i > cur_line; i--) {
                strcpy(editor->lines[i], editor->lines[i - 1]);
            }

            strcpy(editor->lines[cur_line + 1], &editor->lines[cur_line][cur_col]);
            editor->lines[cur_line][cur_col] = '\0';
            editor->line_count++;
            set_cursor_position(editor, 0, cur_line + 1);
        }
    } else if(ch == 8 || ch == 127) {
        if(editor->cursor_x > 0) {
            int col = editor->cursor_x;
            int line = editor->cursor_y;

            memmove(&editor->lines[line][col - 1],
                    &editor->lines[line][col],
                    strlen(&editor->lines[line][col]) + 1);

            set_cursor_position(editor, col - 1, line);
        }
        else if(editor->cursor_y > 0) {
            int cur_line = editor->cursor_y;
            int prev_line = cur_line - 1;
            int new_cursor_x = strlen(editor->lines[prev_line]);

            strcat(editor->lines[prev_line], editor->lines[cur_line]);

            for (int i = cur_line; i < editor->line_count - 1; i++) {
                strcpy(editor->lines[i], editor->lines[i + 1]);
            }

            editor->lines[editor->line_count - 1][0] = '\0';
            editor->line_count--;
            set_cursor_position(editor, new_cursor_x, prev_line);
        }
    } else { 
        int col = editor->cursor_x;
        int line = editor->cursor_y;

        size_t current_len = strlen(editor->lines[line]);
        if(current_len < MAX_LINE_LENGTH - 1) {
            memmove(&editor->lines[line][col + 1],
                &editor->lines[line][col],
                strlen(&editor->lines[line][col]) + 1);

            editor->lines[line][col] = ch;
            set_cursor_position(editor, col + 1, line);
        }
        
    }
}

void state_command_input(EDITOR *editor, char ch, bool input_changed) {
    // If first entry to command mode, initialize buffer
    if (editor->command_buffer[0] == '\0' && ch == ':') {
        editor->command_position = 0;
        return;
    }
    
    // Handle key input
    if (ch == 13) { // Enter - execute command
        editor->command_buffer[editor->command_position] = '\0';
        
        // Process command
        if (strlen(editor->command_buffer) > 0) {
            if (strcmp(editor->command_buffer, "w") == 0) 
                execute_command(editor, cmd_save);
            else if (strcmp(editor->command_buffer, "q") == 0) 
                execute_command(editor, cmd_exit);
            else if (strcmp(editor->command_buffer, "o") == 0) 
                execute_command(editor, cmd_open);
            else if (strcmp(editor->command_buffer, "new") == 0) 
                execute_command(editor, cmd_new);
        }
        
        // Reset and return to normal mode
        editor->command_buffer[0] = '\0';
        editor->command_position = 0;
        editor->state = normal;
    }
    else if (ch == 27) { // Escape - cancel command
        editor->command_buffer[0] = '\0';
        editor->command_position = 0;
        editor->state = normal;
    }
    else if (ch == 8 || ch == 127) { // Backspace
        if (editor->command_position > 0) {
            editor->command_position--;
            editor->command_buffer[editor->command_position] = '\0';
        }
    }
    else if (editor->command_position < MAX_COLUMNS - 1) {
        // Add character to buffer
        if (ch >= 32 && ch <= 126) { // Printable characters
            editor->command_buffer[editor->command_position++] = ch;
            editor->command_buffer[editor->command_position] = '\0';
        }
    }
}

void execute_command(EDITOR* editor, COMMANDS cmd) {
    switch(cmd) {
        case cmd_save: {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
            int msgline = csbi.srWindow.Bottom - 1;
            
            char message[256];
            COORD msg_pos = {0, msgline};
            DWORD written;
            
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            FILE* file = fopen(editor->filepath, "w");
            if (file) {
                for (int i = 0; i < editor->line_count; i++) {
                    fputs(editor->lines[i], file);
                    if (i < editor->line_count - 1) {
                        fputc('\n', file);
                    }
                }
                fclose(file);
                
                editor->is_saved = true;
                strcpy(message, "File saved successfully");
            } else {
                sprintf(message, "Error: Couldn't save file (%s)", strerror(errno));
            }
            
            WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                      msg_pos, &written);
            
            COORD old_pos = {editor->cursor_x, editor->cursor_y};
            Sleep(1500);
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            SetConsoleCursorPosition(editor->current_buffer, old_pos);
            break;
        }
        
        case cmd_exit:
            if(!editor->is_saved) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int msgline = csbi.srWindow.Bottom - 1;
                
                char message[] = "Unsaved changes! Press 'y' to exit anyway, any other key to cancel";
                COORD msg_pos = {0, msgline};
                DWORD written;
                
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                          msg_pos, &written);
                
                int ch = _getch();
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                
                if(ch != 'y' && ch != 'Y') {
                    break;
                }
            }
            exit(0);
            break;

        case cmd_open: {
            if(!editor->is_saved) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int msgline = csbi.srWindow.Bottom - 1;
                
                char message[] = "Unsaved changes! Press 'y' to continue, any other key to cancel";
                COORD msg_pos = {0, msgline};
                DWORD written;
                
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                          msg_pos, &written);
                
                int ch = _getch();
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                
                if(ch != 'y' && ch != 'Y') {
                    break;
                }
            }
            
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
            int msgline = csbi.srWindow.Bottom - 1;
            
            char prompt[] = "Enter full path to open: ";
            COORD msg_pos = {0, msgline};
            DWORD written;
            
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            WriteConsoleOutputCharacter(editor->current_buffer, prompt, strlen(prompt), 
                                      msg_pos, &written);
            
            SetConsoleCursorPosition(editor->current_buffer, (COORD){strlen(prompt), msgline});
            
            char filepath[FILENAME_MAX] = {0};
            int i = 0;
            int ch;
            while(1) {
                ch = _getch();
                if(ch == 13) break;
                if(ch == 27) {
                    filepath[0] = '\0';
                    break;
                }
                if(i < FILENAME_MAX - 1 && ch >= 32 && ch <= 126) {
                    filepath[i++] = ch;
                    char temp[2] = {ch, '\0'};
                    COORD char_pos = {strlen(prompt) + i - 1, msgline};
                    WriteConsoleOutputCharacter(editor->current_buffer, temp, 1, char_pos, &written);
                }
            }
            filepath[i] = '\0';
            
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            if(filepath[0] != '\0') {
                open_create_file(filepath, open);
                exit(0);
            }
            break;
        }

        case cmd_new: {
            if(!editor->is_saved) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int msgline = csbi.srWindow.Bottom - 1;
                
                char message[] = "Unsaved changes! Press 'y' to continue, any other key to cancel";
                COORD msg_pos = {0, msgline};
                DWORD written;
                
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                          msg_pos, &written);
                
                int ch = _getch();
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
                
                if(ch != 'y' && ch != 'Y') {
                    break;
                }
            }
            
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
            int msgline = csbi.srWindow.Bottom - 1;
            
            char prompt[] = "Enter path for new file (or Enter for default): ";
            COORD msg_pos = {0, msgline};
            DWORD written;
            
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            WriteConsoleOutputCharacter(editor->current_buffer, prompt, strlen(prompt), 
                                      msg_pos, &written);
            
            SetConsoleCursorPosition(editor->current_buffer, (COORD){strlen(prompt), msgline});
            
            char filepath[FILENAME_MAX] = {0};
            int i = 0;
            int ch;
            while(1) {
                ch = _getch();
                if(ch == 13) break;
                if(ch == 27) {
                    filepath[0] = '\0';
                    break;
                }
                if(i < FILENAME_MAX - 1 && ch >= 32 && ch <= 126) {
                    filepath[i++] = ch;
                    char temp[2] = {ch, '\0'};
                    COORD char_pos = {strlen(prompt) + i - 1, msgline};
                    WriteConsoleOutputCharacter(editor->current_buffer, temp, 1, char_pos, &written);
                }
            }
            filepath[i] = '\0';
            
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            if(ch != 27) {
                open_create_file(filepath[0] != '\0' ? filepath : NULL, open);
                exit(0);
            }
            break;
        }
    }
}
#include "../include/input.h"
#include "../include/display.h"
#include "../include/fileops.h"

bool process_input(EDITOR* editor) {
    bool input_changed = false;

    if(_kbhit()) {
        int ch = _getch();
        input_changed = true;

        if(editor->state == normal) {
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
            } else if(ch == 'i') {
                editor->state = insert;
            } else if(ch == 'u') {  // Page Up
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
                set_cursor_position(editor, editor->cursor_x, editor->cursor_y - screen_height);
            } else if(ch == 'd') {  // Page Down
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int screen_height = csbi.srWindow.Bottom - csbi.srWindow.Top - 1;
                set_cursor_position(editor, editor->cursor_x, editor->cursor_y + screen_height);
            } else if(ch == 'g') {  // Go to top
                set_cursor_position(editor, 0, 0);
            } else if(ch == 'G') {  // Go to bottom
                set_cursor_position(editor, 0, editor->line_count - 1);
            } else if(ch == ':') {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int cmd_line = csbi.srWindow.Bottom;
                
                char* old_line = (char*)malloc(csbi.dwSize.X + 1);
                DWORD read;
                COORD read_pos = {0, cmd_line};
                ReadConsoleOutputCharacter(editor->current_buffer, old_line, csbi.dwSize.X, read_pos, &read);
                old_line[csbi.dwSize.X] = '\0';
                
                COORD old_cursor_pos = {editor->cursor_x, editor->cursor_y};
                
                char prompt[3] = ":\0";
                COORD cmd_pos = {0, cmd_line};
                DWORD written;
                
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, cmd_pos, &written);
                WriteConsoleOutputCharacter(editor->current_buffer, prompt, strlen(prompt), cmd_pos, &written);
                SetConsoleCursorPosition(editor->current_buffer, (COORD){1, cmd_line});
                
                char cmd[10] = {0};
                int i = 0;
                
                while(1) {
                    ch = _getch();
                    if(ch == 13) break;
                    if(ch == 27) {
                        cmd[0] = '\0';
                        break;
                    }
                    if(i < 9) {
                        cmd[i++] = ch;
                        char temp[2] = {ch, '\0'};
                        WriteConsoleOutputCharacter(editor->current_buffer, temp, 1, 
                                                   (COORD){i, cmd_line}, &written);
                    }
                }
                cmd[i] = '\0';
                
                if(strlen(cmd) > 0) {
                    if(strcmp(cmd, "w") == 0) execute_command(editor, cmd_save);
                    else if(strcmp(cmd, "q") == 0) execute_command(editor, cmd_exit);
                    else if(strcmp(cmd, "o") == 0) execute_command(editor, cmd_open);
                    else if(strcmp(cmd, "new") == 0) execute_command(editor, cmd_new);
                }
                
                if(ch == 27) {
                    WriteConsoleOutputCharacter(editor->current_buffer, old_line, strlen(old_line), 
                                              read_pos, &written);
                    SetConsoleCursorPosition(editor->current_buffer, old_cursor_pos);
                }
                
                free(old_line);
                input_changed = true;
            }
        } else if (editor->state == insert) {
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

                memmove(&editor->lines[line][col + 1],
                        &editor->lines[line][col],
                        strlen(&editor->lines[line][col]) + 1);

                editor->lines[line][col] = ch;
                set_cursor_position(editor, col + 1, line);
            }
        }
    }
    return input_changed;
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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>
#include <conio.h>

#define MAX_LINES 1000
#define MAX_COLUMNS 400

typedef enum _cursor { 
    cur_left,
    cur_right,
    cur_up,
    cur_down,
    cur_enter,
    cur_backspace
} CURSOR;

typedef enum _commands {
    cmd_save,
    cmd_exit,
    cmd_open,
    cmd_new
} COMMANDS;

typedef enum _state {
    normal,
    insert
} STATE;

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

typedef enum _save_open_mode {
    save,
    open
} SAVE_OPEN_MODE;

void init_editor(EDITOR* editor);
bool process_input(EDITOR* editor);
void execute_command(EDITOR* editor, COMMANDS cmd);
void render_editor(EDITOR* editor);
void set_cursor_position(EDITOR* editor, int x, int y);
void show_startup_menu();
void open_create_file(char* filename, SAVE_OPEN_MODE mode);

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        show_startup_menu();
    } else {
        open_create_file(argv[1], open);
    }
    
    return 0;
}

void init_editor(EDITOR* editor) {
    memset(editor->lines, 0, sizeof(editor->lines));
    editor->lines[0][0] = '\0';
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
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

void render_editor(EDITOR* editor) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);

    int cur_x = editor->cursor_x;
    int cur_y = editor->cursor_y;

    DWORD dw_bytes_written = 0;
    COORD cursor_pos = {0, 0};

    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(editor->new_buffer, ' ', length, cursor_pos, &dw_bytes_written);
    FillConsoleOutputAttribute(editor->new_buffer, csbi.wAttributes, length, cursor_pos, &dw_bytes_written);

    for(int i = 0; i < editor->line_count; i++) {
        cursor_pos.Y = i;
        cursor_pos.X = 0;
        WriteConsoleOutputCharacter(editor->new_buffer, editor->lines[i], 
                                   strlen(editor->lines[i]), cursor_pos, &dw_bytes_written);
    }
    
    int last_line = csbi.srWindow.Bottom;
    COORD status_pos = {0, last_line};
    
    char status_text[256];
    sprintf(status_text, " %-10s %s",
            editor->state == normal ? "NORMAL" : "INSERT", editor->filename);
    
    WriteConsoleOutputCharacter(editor->new_buffer, status_text, 
                              strlen(status_text), status_pos, &dw_bytes_written);

    HANDLE temp = editor->current_buffer;
    editor->current_buffer = editor->new_buffer;
    editor->new_buffer = temp;

    SetConsoleActiveScreenBuffer(editor->current_buffer);
    SetConsoleCursorPosition(editor->current_buffer, (COORD){cur_x, cur_y});
}

void set_cursor_position(EDITOR* editor, int x, int y) {
    if (y >= editor->line_count) {
        y = editor->line_count - 1;
    }
    if (y < 0) {
        y = 0;
    }
    
    int line_length = strlen(editor->lines[y]);
    if (x > line_length) {
        x = line_length;
    }
    if (x < 0) {
        x = 0;
    }
    
    editor->cursor_x = x;
    editor->cursor_y = y;
    SetConsoleCursorPosition(editor->current_buffer, (COORD){x, y});
}

void show_startup_menu() {
    SetConsoleOutputCP(65001);
    
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console, &csbi);
    int width = csbi.dwSize.X;
    int height = csbi.dwSize.Y;

    system("cls");
    SetConsoleCursorPosition(console, (COORD){0, 0});

    int start_y = height / 4;

    const char* header[] = {
        "  ████████╗██╗    ██╗██████╗     ███████╗██████╗ ██╗████████╗ ██████╗ ██████╗ ",
        "  ╚══██╔══╝██║    ██║██╔══██╗    ██╔════╝██╔══██╗██║╚══██╔══╝██╔═══██╗██╔══██╗",
        "     ██║   ██║ █╗ ██║██████╔╝    █████╗  ██║  ██║██║   ██║   ██║   ██║██████╔╝",
        "     ██║   ██║███╗██║██╔═══╝     ██╔══╝  ██║  ██║██║   ██║   ██║   ██║██╔══██╗",
        "     ██║   ╚███╔███╔╝██║         ███████╗██████╔╝██║   ██║   ╚██████╔╝██║  ██║",
        "     ╚═╝     ╚═╝ ╚═╝ ╚═╝         ╚══════╝╚═════╝ ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝"
    };

    int header_lines = sizeof(header) / sizeof(header[0]);

    for(int i = 0; i < header_lines; i++) {
        int text_len = 78;
        int start_x = (width - text_len) / 2;
        if(start_x < 0) start_x = 0;
        
        SetConsoleCursorPosition(console, (COORD){start_x, start_y + i});
        
        DWORD written;
        WriteConsoleA(console, header[i], strlen(header[i]), &written, NULL);
    }

    const char* version = "Version 0.1";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(version)) / 2, start_y + header_lines + 1});
    printf("%s", version);

    const char* subtitle = "A Vim-like Terminal Text Editor";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(subtitle)) / 2, start_y + header_lines + 2});
    printf("%s", subtitle);

    const char* menu[] = {
        "",
        "  New File       [n]",
        "  Open File      [o]",
        "  Exit           [q]",
        "",
        "  Help           [h]",
        ""
    };

    int menu_items = sizeof(menu) / sizeof(menu[0]);

    for(int i = 0; i < menu_items; i++) {
        int text_len = strlen(menu[i]);
        int start_x = (width - text_len) / 2;
        SetConsoleCursorPosition(console, (COORD){start_x, start_y + header_lines + 4 + i});
        printf("%s", menu[i]);
    }

    const char* footer = "TWP Editor by David Radoslav";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(footer)) / 2, start_y + header_lines + 4 + menu_items + 2});
    printf("%s", footer);

    while(true) {
        int ch = _getch();
        switch(ch) {
            case 'n': {
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0});
                printf("Enter path for new file (can include directories): ");
                
                char filepath[FILENAME_MAX] = {0};
                fflush(stdin);
                fgets(filepath, FILENAME_MAX, stdin);
                
                size_t len = strlen(filepath);
                if (len > 0 && filepath[len-1] == '\n') {
                    filepath[len-1] = '\0';
                    len--;
                }
                
                if (len > 0) {
                    open_create_file(filepath, open);
                } else {
                    open_create_file(NULL, open);
                }
                return;
            }
            case 'o': {
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0});
                printf("Enter full path to file: ");
                
                char filepath[FILENAME_MAX] = {0};
                scanf("%s", filepath);
                open_create_file(filepath, open);
                return;
            }
            case 'h':
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0});
                printf("Help not implemented yet\n");
                break;
            case 'q':
                exit(0);
        }
    }
}

void open_create_file(char* filepath, SAVE_OPEN_MODE mode) {
    system("cls");
    
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(console, (COORD){0, 0});
    
    EDITOR editor;
    memset(&editor, 0, sizeof(EDITOR));
    
    if (filepath && strlen(filepath) > 0) {
        strcpy(editor.filepath, filepath);
    }
    
    init_editor(&editor);
    
    if (filepath && strlen(filepath) > 0) {
        FILE* file = fopen(filepath, "r");
        if (file) {
            int line = 0;
            while (line < MAX_LINES && fgets(editor.lines[line], MAX_COLUMNS, file)) {
                size_t len = strlen(editor.lines[line]);
                if (len > 0 && editor.lines[line][len-1] == '\n') {
                    editor.lines[line][len-1] = '\0';
                }
                line++;
            }
            editor.line_count = line > 0 ? line : 1;
            fclose(file);
            
            editor.is_saved = true;
        } else {
            editor.is_saved = false;
            editor.line_count = 1;
            editor.lines[0][0] = '\0';
        }
    }
    
    render_editor(&editor);
    
    bool needs_redraw = false;
    while (true) {
        needs_redraw = process_input(&editor);
        if (needs_redraw) {
            render_editor(&editor);
        }
        Sleep(50);
    }
}
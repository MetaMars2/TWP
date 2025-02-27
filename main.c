//Includes
#include <errno.h> // Error handling
#include <stdlib.h> // Standard library
#include <stdio.h> // FILENAME_MAX, printf()
#include <stdbool.h> // bool, true, false
#include <string.h> // strcpy()
// Windows specific includes
#include <windows.h> //CONSOLE_SCREEN_BUFFER_INFO, GetConsoleScreenBufferInfo(), GetStdHandle(), STD_OUTPUT_HANDLE
#include <conio.h> // _kbhit(), _getch()

#define MAX_LINES 1000 // Maximum number of lines in the file
#define MAX_COLUMNS 400 // Maximum number of columns in the file

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
    int line_count; // Number of lines in the file
    int cursor_x; 
    int cursor_y; 
    STATE state; // Current state of the editor
    char filename[FILENAME_MAX];
    bool is_saved;
    HANDLE current_buffer; // active buffer
    HANDLE new_buffer; // new buffer
} EDITOR;

typedef enum _save_open_mode{
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
    // Initialize basic buffers
    memset(editor->lines, 0, sizeof(editor->lines));
    editor->lines[0][0] = '\0';
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->state = normal;
    
    // Initialize filename
    if (editor->filename[0] == '\0') {
        strcpy(editor->filename, "untitled.txt");
    } else {
        // Extract filename from path if needed
        char* filename_part = strrchr(editor->filename, '\\');
        if (filename_part) {
            // If we found a backslash, use the part after it
            strcpy(editor->filename, filename_part + 1);
        }
        // Otherwise keep the original name (it's just a filename)
    }
    
    // Check if file exists to determine save status
    FILE* file = fopen(editor->filename, "r");
    if (file) {
        // File exists - compare with current contents to determine save status
        char file_contents[MAX_LINES][MAX_COLUMNS];
        int line_count = 0;
        
        while (line_count < MAX_LINES && fgets(file_contents[line_count], MAX_COLUMNS, file)) {
            // Remove trailing newline
            size_t len = strlen(file_contents[line_count]);
            if (len > 0 && file_contents[line_count][len-1] == '\n') {
                file_contents[line_count][len-1] = '\0';
            }
            line_count++;
        }
        fclose(file);
        
        // If there's content in the editor, compare with file contents
        if (editor->line_count > 1 || strlen(editor->lines[0]) > 0) {
            editor->is_saved = false; // Assume not saved unless proven otherwise
            
            // Only mark as saved if line count matches and all lines are identical
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
            // Empty editor opening existing file - it's considered saved
            editor->is_saved = true;
        }
    } else {
        // File doesn't exist - editor is considered unsaved if it has content
        editor->is_saved = (editor->line_count == 1 && editor->lines[0][0] == '\0');
    }

    // Screen buffer initialization
    editor->current_buffer = GetStdHandle(STD_OUTPUT_HANDLE);
    editor->new_buffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
    
    // Set console mode for the buffers
    DWORD mode;
    GetConsoleMode(editor->current_buffer, &mode);
    SetConsoleMode(editor->current_buffer, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleMode(editor->new_buffer, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    // Configure cursor appearance
    CONSOLE_CURSOR_INFO cursor_info = {100, TRUE}; // Size 100%, visible
    SetConsoleCursorInfo(editor->current_buffer, &cursor_info);
    SetConsoleCursorInfo(editor->new_buffer, &cursor_info);
}

bool process_input(EDITOR* editor){
    bool input_changed = false;

    if(_kbhit()){
        int ch = _getch();
        input_changed = true;

        if(editor->state == normal){
            // Handle vim-like movement keys
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
            } else if(ch == 'i'){
                editor->state = insert;
            } else if(ch == ':'){
                // Command mode
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
                int cmd_line = csbi.srWindow.Bottom;
                
                // Save the current state to restore later
                char* old_line = (char*)malloc(csbi.dwSize.X + 1);
                DWORD read;
                COORD read_pos = {0, cmd_line};
                ReadConsoleOutputCharacter(editor->current_buffer, old_line, csbi.dwSize.X, read_pos, &read);
                old_line[csbi.dwSize.X] = '\0';
                
                COORD old_cursor_pos = {editor->cursor_x, editor->cursor_y};
                
                // Prepare command prompt in buffer
                char prompt[3] = ":\0";
                COORD cmd_pos = {0, cmd_line};
                DWORD written;
                
                // Clear the command line
                FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, cmd_pos, &written);
                
                // Write the prompt
                WriteConsoleOutputCharacter(editor->current_buffer, prompt, strlen(prompt), cmd_pos, &written);
                
                // Position cursor
                SetConsoleCursorPosition(editor->current_buffer, (COORD){1, cmd_line});
                
                // Get command input
                char cmd[10] = {0};
                int i = 0;
                
                // Read command chars
                while(1){
                    ch = _getch();
                    if(ch == 13) break; // Enter key
                    if(ch == 27) { // Escape key - cancel command
                        cmd[0] = '\0';
                        break;
                    }
                    if(i < 9) {
                        cmd[i++] = ch;
                        // Echo the character to the buffer
                        char temp[2] = {ch, '\0'};
                        WriteConsoleOutputCharacter(editor->current_buffer, temp, 1, 
                                                   (COORD){i, cmd_line}, &written);
                    }
                }
                cmd[i] = '\0';
                
                // Process command
                if(strlen(cmd) > 0) {
                    if(strcmp(cmd, "w") == 0) execute_command(editor, cmd_save);
                    else if(strcmp(cmd, "q") == 0) execute_command(editor, cmd_exit);
                    else if(strcmp(cmd, "o") == 0) execute_command(editor, cmd_open);
                    else if(strcmp(cmd, "new") == 0) execute_command(editor, cmd_new);
                }
                
                // Restore the command line
                if(ch == 27) { // Only restore if ESC was pressed
                    WriteConsoleOutputCharacter(editor->current_buffer, old_line, strlen(old_line), 
                                              read_pos, &written);
                    SetConsoleCursorPosition(editor->current_buffer, old_cursor_pos);
                } else {
                    // For successful commands, let render_editor handle the redraw
                }
                
                free(old_line);
                input_changed = true;
            }
        } else if (editor->state == insert){
            // In insert mode, accept typing and check for ESC to exit
            if(ch == 27){ // ESC key
                editor->state = normal;
            } else if(ch == 0 || ch == 224) { // Special key sequence (arrow keys)
                // Get the second byte of the sequence
                ch = _getch();
                
                // Handle arrow keys in insert mode
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
            } else if(ch == 13){ // Enter key
                // Move the cursor to the next line
                int cur_line = editor->cursor_y;
                int cur_col = editor->cursor_x;

                if(editor->line_count < MAX_LINES){
                    for(int i = editor->line_count; i > cur_line; i--){
                        strcpy(editor->lines[i], editor->lines[i - 1]);
                    }

                    // Copy remainder of lines to the next line
                    strcpy(editor->lines[cur_line + 1], &editor->lines[cur_line][cur_col]);

                    // Clear the current line after the cursor position
                    editor->lines[cur_line][cur_col] = '\0';

                    editor->line_count++;
                    set_cursor_position(editor, 0, cur_line + 1);
                }

                

            } else if( ch == 8 || ch == 127){ //backspace key
                // Only delete if we're not at the beginning of the line
                if(editor->cursor_x > 0){
                    int col = editor->cursor_x;
                    int line = editor->cursor_y;

                    // Remove characters by shifting everything after it
                    memmove(&editor->lines[line][col - 1],
                            &editor->lines[line][col],
                            strlen(&editor->lines[line][col]) + 1);

                    // Move the cursor back
                    set_cursor_position(editor, col - 1, line);
                }
                // Handle backspace at beginning of line (join with previous line)
                else if(editor->cursor_y > 0){ // Beginning of a line (but not the first line)
                    int cur_line = editor->cursor_y;
                    int prev_line = cur_line - 1;

                    // Calculate the new cursor position (end of previous line)
                    int new_cursor_x = strlen(editor->lines[prev_line]);

                    // Append the current line to the previous line
                    strcat(editor->lines[prev_line], editor->lines[cur_line]);

                    // Shift all subsequent lines up to fill the gap
                    for (int i = cur_line; i < editor->line_count - 1; i++) {
                        strcpy(editor->lines[i], editor->lines[i + 1]);
                    }

                    // Clear the last line
                    editor->lines[editor->line_count - 1][0] = '\0';

                    // Decrease the line count
                    editor->line_count--;

                    // Move the cursor to the join position
                    set_cursor_position(editor, new_cursor_x, prev_line);
                }

            } else { 
                int col = editor->cursor_x;
                int line = editor->cursor_y;

                // Shift characters at curent position
                memmove(&editor->lines[line][col + 1],
                        &editor->lines[line][col],
                        strlen(&editor->lines[line][col]) + 1);

                // Insert the character
                editor->lines[line][col] = ch;
                
                // Use set_cursor_position instead of directly incrementing
                set_cursor_position(editor, col + 1, line);
            }
        }
    }
    return input_changed;
}

void execute_command(EDITOR* editor, COMMANDS cmd){
    switch(cmd){
        case cmd_save:{
            // Get console info
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);
            int msgline = csbi.srWindow.Bottom - 1;
            
            // Prepare message buffer
            char message[256];
            COORD msg_pos = {0, msgline};
            DWORD written;
            
            // Clear the message line first
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            // Try to save the file
            FILE* file = fopen(editor->filename, "w");
            if (file) {
                // Write each line to the file
                for (int i = 0; i < editor->line_count; i++) {
                    fputs(editor->lines[i], file);
                    if (i < editor->line_count - 1) {
                        fputc('\n', file); // Add newline between lines (not after last line)
                    }
                }
                fclose(file);
                
                // Mark as saved
                editor->is_saved = true;
                strcpy(message, "File saved successfully");
            } else {
                // Handle error - couldn't open file for writing
                sprintf(message, "Error: Couldn't save file (%s)", strerror(errno));
            }
            
            // Write the message to the buffer
            WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                      msg_pos, &written);
            
            // Save cursor position
            COORD old_pos = {editor->cursor_x, editor->cursor_y};
            
            // Let the message stay visible for a moment
            Sleep(1500);
            
            // Clear the message
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            // Restore cursor position
            SetConsoleCursorPosition(editor->current_buffer, old_pos);
            break;
        }
        // Other cases remain the same
        case cmd_exit:
            if(editor->is_saved){ // TODO: confirm_exit() needs to be implemented
                exit(0);
            }
            exit(0);
            break;
        case cmd_open:
            // TODO: Open file implementation
            break;
        case cmd_new:
            // TODO: New file implementation
            break;
    }
}

void render_editor(EDITOR* editor) {
    // Get console info
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(editor->current_buffer, &csbi);

    // Store current cursor position
    int cur_x = editor->cursor_x;
    int cur_y = editor->cursor_y;

    // Write content to the buffer that isn't currently active
    DWORD dw_bytes_written = 0;
    COORD cursor_pos = {0, 0};

    // Clear the buffer
    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(editor->new_buffer, ' ', length, cursor_pos, &dw_bytes_written);
    FillConsoleOutputAttribute(editor->new_buffer, csbi.wAttributes, length, cursor_pos, &dw_bytes_written);

    // Write each line to the buffer
    for(int i = 0; i < editor->line_count; i++) {
        cursor_pos.Y = i;
        cursor_pos.X = 0;
        WriteConsoleOutputCharacter(editor->new_buffer, editor->lines[i], 
                                   strlen(editor->lines[i]), cursor_pos, &dw_bytes_written);
    }
    
    // Render status bar to the new buffer
    int last_line = csbi.srWindow.Bottom;
    COORD status_pos = {0, last_line};
    
    // Create status text
    char status_text[256];
    sprintf(status_text, " %-10s %s",
            editor->state == normal ? "NORMAL" : "INSERT", editor->filename);
    
    // Write status text to the new buffer
    WriteConsoleOutputCharacter(editor->new_buffer, status_text, 
                              strlen(status_text), status_pos, &dw_bytes_written);

    // Swap buffers
    HANDLE temp = editor->current_buffer;
    editor->current_buffer = editor->new_buffer;
    editor->new_buffer = temp;

    // Make the new buffer active
    SetConsoleActiveScreenBuffer(editor->current_buffer);
    
    // Restore cursor position after buffer swap
    SetConsoleCursorPosition(editor->current_buffer, (COORD){cur_x, cur_y});
}

void set_cursor_position(EDITOR* editor, int x, int y) {
    // Ensure we don't go beyond valid boundaries
    if (y >= editor->line_count) {
        y = editor->line_count - 1;
    }
    if (y < 0) {
        y = 0;
    }
    
    // Limit x position to line length if needed
    int line_length = strlen(editor->lines[y]);
    if (x > line_length) {
        x = line_length;
    }
    if (x < 0) {
        x = 0;
    }
    
    editor->cursor_x = x;
    editor->cursor_y = y;
    
    // Update the active console cursor immediately
    SetConsoleCursorPosition(editor->current_buffer, (COORD){x, y});
}

void show_startup_menu(){
    // Set console to UTF-8 mode and proper font
    SetConsoleOutputCP(65001);
    
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console, &csbi);
    int width = csbi.dwSize.X;
    int height = csbi.dwSize.Y;

    system("cls");
    SetConsoleCursorPosition(console, (COORD){0, 0});

    // Calculate center position for text
    int start_y = height / 4;

    // Use the commented out header since it's shorter and more likely to display correctly
    const char* header[] = {
        "  ████████╗██╗    ██╗██████╗     ███████╗██████╗ ██╗████████╗ ██████╗ ██████╗ ",
        "  ╚══██╔══╝██║    ██║██╔══██╗    ██╔════╝██╔══██╗██║╚══██╔══╝██╔═══██╗██╔══██╗",
        "     ██║   ██║ █╗ ██║██████╔╝    █████╗  ██║  ██║██║   ██║   ██║   ██║██████╔╝",
        "     ██║   ██║███╗██║██╔═══╝     ██╔══╝  ██║  ██║██║   ██║   ██║   ██║██╔══██╗",
        "     ██║   ╚███╔███╔╝██║         ███████╗██████╔╝██║   ██║   ╚██████╔╝██║  ██║",
        "     ╚═╝     ╚═╝ ╚═╝ ╚═╝         ╚══════╝╚═════╝ ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝"
    };

    int header_lines = sizeof(header) / sizeof(header[0]);

    // Print header centered - fixed method for UTF-8
    for(int i = 0; i < header_lines; i++){
        // Use a fixed display width - each line is the same width
        int text_len = 78; // Hardcoded width of the ASCII art
        int start_x = (width - text_len) / 2;
        if(start_x < 0) start_x = 0; // Prevent negative position
        
        SetConsoleCursorPosition(console, (COORD){start_x, start_y + i});
        
        // Write to console output directly for better control
        DWORD written;
        WriteConsoleA(console, header[i], strlen(header[i]), &written, NULL);
    }

    // Version info
    const char* version = "Version 0.1";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(version)) / 2, start_y + header_lines + 1});
    printf("%s", version);

    // Subtitle
    const char* subtitle = "A Vim-like Terminal Text Editor";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(subtitle)) / 2, start_y + header_lines + 2});
    printf("%s", subtitle);

    // Menu options
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

    // Print menu 
    for(int i = 0; i < menu_items; i++){
        int text_len = strlen(menu[i]);
        int start_x = (width - text_len) / 2;
        SetConsoleCursorPosition(console, (COORD){start_x, start_y + header_lines + 4 + i});
        printf("%s", menu[i]);
    }

    // Footer
    const char* footer = "TWP Editor by David Radoslav";
    SetConsoleCursorPosition(console, (COORD){(width - strlen(footer)) / 2, start_y + header_lines + 4 + menu_items + 2});
    printf("%s", footer);

    // Handle input
    while(true){
        int ch = _getch();
        switch(ch){
            case 'n': { // new file
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0});
                printf("Enter name for new file: ");
                
                char filepath[FILENAME_MAX] = {0};
                // Use fgets instead of scanf to handle empty input (just pressing Enter)
                fflush(stdin);
                fgets(filepath, FILENAME_MAX, stdin);
                
                // Remove trailing newline if present
                size_t len = strlen(filepath);
                if (len > 0 && filepath[len-1] == '\n') {
                    filepath[len-1] = '\0';
                    len--;
                }
                
                // If the user entered a filename, use it; otherwise pass NULL for default
                if (len > 0) {
                    open_create_file(filepath, open);
                } else {
                    open_create_file(NULL, open);
                }
                return;
            }
            case 'o': { // Open file
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0});
                printf("Enter filepath to open: ");
                
                char filepath[FILENAME_MAX] = {0};
                scanf("%s", filepath);
                open_create_file(filepath, open);
                return;
            }
            case 'h': // Help
                system("cls");
                SetConsoleCursorPosition(console, (COORD){0, 0}); // TODO: Implement help docs
                printf("Help not implemented yet\n");
                break;
            case 'q': // Quit
                exit(0);
        }
    }
}

void open_create_file(char* filename, SAVE_OPEN_MODE mode){
    // Clear the screen
    system("cls");
    
    // Get console handle
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Move cursor to top left
    SetConsoleCursorPosition(console, (COORD){0, 0});
    
    // Create new editor
    EDITOR editor;
    init_editor(&editor);
    
    // If filename was provided and not NULL, try to load it
    if (filename && strlen(filename) > 0) {
        FILE* file = fopen(filename, "r");
        if (file) {
            // Load file content into editor
            int line = 0;
            while (line < MAX_LINES && fgets(editor.lines[line], MAX_COLUMNS, file)) {
                // Remove trailing newline if present
                size_t len = strlen(editor.lines[line]);
                if (len > 0 && editor.lines[line][len-1] == '\n') {
                    editor.lines[line][len-1] = '\0';
                }
                line++;
            }
            editor.line_count = line > 0 ? line : 1;
            fclose(file);
            
            // Set filename
            strcpy(editor.filename, filename);
            editor.is_saved = true;
        } else {
            // New file with given name
            strcpy(editor.filename, filename);
            editor.is_saved = false;
            editor.line_count = 1;
            editor.lines[0][0] = '\0';
        }
    } else {
        // New file with default name
        strcpy(editor.filename, "untitled.txt");
        editor.is_saved = false;
        editor.line_count = 1;
        editor.lines[0][0] = '\0';
    }
    
    // Start editor loop
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
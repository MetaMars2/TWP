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

void init_editor(EDITOR* editor);

bool process_input(EDITOR* editor);
CURSOR get_cursor_input();
COMMANDS get_command_input(STATE state);
void execute_command(EDITOR* editor, COMMANDS cmd);
void render_editor(EDITOR* editor);
void set_cursor_position(EDITOR* editor, int x, int y);




int main(int argc, char* argv[]) {


    if(argc <= 1){
        // TODO: show_startup_menu();
    } else {
        // TODO: open_create_file(argv[1]);
    }


    // Clear the screen
    system("cls");

    // Move the cursor to the top left corner
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){0, 0});

    EDITOR editor;
    init_editor(&editor);

    render_editor(&editor);

    bool needs_redraw = false;

    while(true){
        // Process input first
        needs_redraw = process_input(&editor);
        
        // Only redraw if something changed
        if(needs_redraw){
            render_editor(&editor);
        }

        // Longer sleep to reduce CPU usage and flickering
        Sleep(50);
    }

    return 0;
}

void init_editor(EDITOR* editor) {
    editor->lines[0][0] = '\0';
    editor->line_count = 1;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->state = normal;
    strcpy(editor->filename, "meow.txt"); // TODO: Set the filename
    editor->is_saved = true; // TODO: Check if the file is saved

    // Screen buffer
    editor->current_buffer = GetStdHandle(STD_OUTPUT_HANDLE);
    editor->new_buffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
}



CURSOR get_cursor_input(){
    int ch = _getch();

    switch(ch){
        case 'h': return cur_left;
        case 'j': return cur_down;
        case 'k': return cur_up;
        case 'l': return cur_right;
        case 13:  return cur_enter; // Enter key
        case 8:   return cur_backspace; // Backspace key
        default:  return -1; // Not a cursor movement
    }

}

COMMANDS get_command_input(STATE state){
    // In normal mode, comands are often prefixed with ':'
    if(state == normal){
        int ch = _getch();
        if(ch == ':'){
            char cmd [10] = {0};
            int i = 0;

            while((ch = _getch()) != 13 && i < 9){ // Read until Enter
                putchar(ch);
                cmd[i++] = ch;
            }
            cmd[i] = '\0';

            if(strcmp(cmd, "w") == 0) return cmd_save;
            if(strcmp(cmd, "q") == 0) return cmd_exit;
            if(strcmp(cmd, "o") == 0) return cmd_open;
            if(strcmp(cmd, "new") == 0) return cmd_new;
        }
    }
    return -1;
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
            
            // Prepare message
            char message[] = "File saved successfully";
            COORD msg_pos = {0, msgline};
            DWORD written;
            
            // Clear the message line first
            FillConsoleOutputCharacter(editor->current_buffer, ' ', csbi.dwSize.X, msg_pos, &written);
            
            // Write the message to the buffer
            WriteConsoleOutputCharacter(editor->current_buffer, message, strlen(message), 
                                      msg_pos, &written);
            
            editor->is_saved = true;
            
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
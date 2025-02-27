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
    char lines[MAX_LINES][MAX_COLUMNS]; // Array of lines
    int line_count; // Number of lines in the file
    int cursor_x; // Cursor x position
    int cursor_y; // Cursor y position
    STATE state; // Current state of the editor
    char filename[FILENAME_MAX]; // Filename of the file
    bool is_saved; // Is the file saved
} EDITOR;

void init_editor(EDITOR* editor);
void render_status_bar(EDITOR* editor, STATE STATE);
bool process_input(EDITOR* editor);
CURSOR get_cursor_input();
COMMANDS get_command_input(STATE state);
void execute_command(EDITOR* editor, COMMANDS cmd);
void render_editor(EDITOR* editor);



int main(int argc, char* argv[]) {


    if(argc <= 1){
        // TODO: show_startup_menu();
    } else {
        // TODO: open_create_file(argv[1]);
    }


    printf("\033[2J"); // Clear the screen
    printf("\033[H"); // Move the cursor to the top left corner

    EDITOR editor;
    init_editor(&editor);

    render_editor(&editor);
    render_status_bar(&editor, editor.state);

    bool needs_redraw = false;

    while(true){
        // Process input first
        needs_redraw = process_input(&editor);
        
        // Only redraw if something changed
        if(needs_redraw){
            render_editor(&editor);
            render_status_bar(&editor, editor.state);
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
}

void render_status_bar(EDITOR* editor, STATE STATE) {
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int last_line = csbi.srWindow.Bottom;
    
    // Clear the entire status line first
    printf("\033[%d;0H\033[K", last_line);

    // Then print the status information
    printf("\033[%d;0H %s\t\t%s", last_line,
            STATE == normal ? "NORMAL" : "INSERT", editor->filename);

    // Move cursor back to editing position
    printf("\033[%d;%dH", editor->cursor_y + 1, editor->cursor_x + 1);

    fflush(stdout);
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
                editor->cursor_x--;
            } else if(ch == 'l' ) {
                size_t line_lenght = strlen(editor->lines[editor->cursor_y]);
                if(editor->cursor_x < line_lenght) {
                    editor->cursor_x++;
                }
            } else if(ch == 'k' && editor->cursor_y > 0) {
                editor->cursor_y--;
            } else if(ch == 'j' && editor->cursor_y < editor->line_count - 1) {
                editor->cursor_y++;
            } else if(ch == 'i'){
                editor->state = insert;
            } else if(ch == ':'){
                // Command mode
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
                int cmd_line = csbi.srWindow.Bottom;

                // Show command prompt
                printf("\033[%d;0H:", cmd_line);

                // Get command
                char cmd[10] = {0};
                int i = 0;
                while((ch = _getch()) != 13 && i < 9){
                    putchar(ch);
                    cmd[i++] = ch;
                }
                cmd[i] = '\0';

                // Process command
                if (strcmp(cmd, "w") == 0) execute_command(editor, cmd_save);
                else if (strcmp(cmd, "q") == 0) execute_command(editor, cmd_exit);
                else if (strcmp(cmd, "o") == 0) execute_command(editor, cmd_open);
                else if (strcmp(cmd, "new") == 0) execute_command(editor, cmd_new);
            }
        } else if (editor->state == insert){
            //In insert mode, accept typing and check for ESC to exit
            if(ch == 27){ // ESC key
                editor->state = normal;
            } else if(ch == 13){ // Enter key
                // TODO: Handle new line insertion
                // Move the cursor to the next line

            } else if( ch == 8 || ch == 127){ //bacspace key
                // Only delete if we're not at the beginning of the line
                if(editor->cursor_x > 0){
                    int col = editor->cursor_x;
                    int line = editor->cursor_y;

                    // Remove characters by shifting everything after it
                    memmove(&editor->lines[line][col - 1],
                            &editor->lines[line][col],
                            strlen(&editor->lines[line][col]) + 1);

                    // Move the cursor back
                    editor->cursor_x--;
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
                editor->cursor_x++;

                // ? Is there more todo here?
            }
        }
    }
    return input_changed;
}

void execute_command(EDITOR* editor, COMMANDS cmd){
    switch(cmd){
        case cmd_save:{
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            int msgline = csbi.srWindow.Bottom - 1;

            printf("\033[%d;0H\033[K", msgline);
            printf("File save successfully"); // TODO: Save file implementation & file save confirmation

            editor->is_saved = true;
            Sleep(1500);
            break;
        }
        case cmd_exit:
            if(editor->is_saved ){ // TODO: confirm_exit() needs to be implemented
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

void render_editor(EDITOR* editor){
    // Clear the screen
    printf("\033[2J");
    printf("\033[H");

    // Render the editor
    for(int i = 0; i < editor->line_count; i++){
        printf("%s\n", editor->lines[i]);
    }
}
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
void process_input(EDITOR* editor);
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


    printf("\033[2j"); // Clear the screen
    printf("\033[H"); // Move the cursor to the top left corner

    EDITOR editor;
    init_editor(&editor);

    while(true){
        render_editor(&editor);
        render_status_bar(&editor, editor.state);
        process_input(&editor);
        Sleep(1);
    }

    return 0;
}

void init_editor(EDITOR* editor) {
    editor->line_count = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->state = normal;
    strcpy(editor->filename, "meow.txt");
    editor->is_saved = true;
}

void render_status_bar(EDITOR* editor, STATE STATE) {
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int lastline;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    lastline = csbi.srWindow.Bottom;
    // Output: NORMAL/INSERT        filename
    printf("\033[%d;0H %s\t\t%s", lastline, STATE == normal ? "NORMAL" : "INSERT", editor->filename);
    // Move the cursor to the previous position
    printf("\033[%d;%dH", editor->cursor_y + 1, editor->cursor_x + 1);
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

void process_input(EDITOR* editor){
    if(_kbhit()){
        if(editor->state == normal){
            // In normal mode, chech for both cursor movements and commands
            CURSOR cursor = get_cursor_input();
            if(cursor != -1){
                // Handle cursor movement
                switch(cursor){
                    case cur_left: if(editor->cursor_x > 0) editor->cursor_x--; break;
                    case cur_right: {
                        size_t last_ch_index = strlen(editor->lines[editor->cursor_y]);
                        if(editor->cursor_x < last_ch_index) editor->cursor_x++; 
                        else if(editor->cursor_y < editor->line_count) {
                            editor->cursor_x = 0;
                            editor->cursor_y++;
                        } 
                        break;
                    }
                    case cur_up: if(editor->cursor_y > 0) editor->cursor_y--; break;
                    case cur_down: if(editor->cursor_y < editor->line_count-1) editor->cursor_y++; break;
                }
            } else {
                // Check for mode change to insert
                int ch = _getch();
                if(ch == 'i'){
                    editor->state = insert;
                } else if(ch == ':'){
                    // Display the colon first
                    printf(":");

                    // Get the command string
                    char cmd[10] = {0};
                    int i = 0;
                    
                    while((ch = _getch()) != 13 && i < 9){ // Read until Enter
                        putchar(ch);
                        cmd[i++] = ch;
                    }
                    cmd[i] = '\0';

                    if(strcmp(cmd, "w") == 0) execute_command(editor, cmd_save);
                    if(strcmp(cmd, "q") == 0) execute_command(editor, cmd_exit);
                    if(strcmp(cmd, "o") == 0) execute_command(editor, cmd_open);
                    if(strcmp(cmd, "new") == 0) execute_command(editor, cmd_new);
                }
            }
        } else if (editor->state == insert){
            //In insert mode, accept typing and check for ESC to exit
            int ch = _getch();
            if(ch == 27){ // ESC key
                editor->state = normal;
            } else {
                // TODO: Handle typing insertion
            }
        }
    }
}

void execute_command(EDITOR* editor, COMMANDS cmd){
    switch(cmd){
        case cmd_save:
            // TODO: Save file implementation
            printf("\nFile saved");
            Sleep(1000);
            editor->is_saved = true;
            break;
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
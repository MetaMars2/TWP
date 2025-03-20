#include "../include/fileops.h"
#include "../include/display.h"
#include "../include/input.h"

void open_create_file(char* filepath) {
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
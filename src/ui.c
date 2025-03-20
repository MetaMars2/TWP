#include "../include/ui.h"
#include "../include/fileops.h"

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
        "     ╚═╝    ╚══╝╚══╝ ╚═╝         ╚══════╝╚═════╝ ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝"
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
        menu_selection(console);
    }
}

void menu_selection(HANDLE console){
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
                open_create_file(filepath);
            } else {
                open_create_file(NULL);
            }
            return;
        }
        case 'o': {
            system("cls");
            SetConsoleCursorPosition(console, (COORD){0, 0});
            printf("Enter full path to file: ");
            
            char filepath[FILENAME_MAX] = {0};
            scanf("%s", filepath);
            open_create_file(filepath);
            return;
        }
        case 'h': {
            system("cls");
            SetConsoleCursorPosition(console, (COORD){0, 0});
            printf("Help not implemented yet\n");
            Sleep(1500);
            show_startup_menu(); // Redraw menu
            return;
        }
        case 'q':
            exit(0);
    }


}
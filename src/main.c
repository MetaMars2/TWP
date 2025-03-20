#include "../include/common.h"
#include "../include/ui.h"
#include "../include/fileops.h"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        show_startup_menu();
    } else {
        open_create_file(argv[1]);
    }
    
    return 0;
}
#include "../include/common.h"
#include "../include/ui.h"
#include "../include/fileops.h"
#include "../include/config.h"

int main(int argc, char* argv[]) {
    // Ensure config file exists
    FILE* config_test = fopen("twp.conf", "r");
    if (!config_test) {
        // Create default config
        Config* default_config = init_default_config();
        save_config(default_config);
        free_config(default_config);
    } else {
        fclose(config_test);
    }
    
    if (argc <= 1) {
        show_startup_menu();
    } else {
        open_create_file(argv[1], open);
    }
    
    return 0;
}
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Config file path relative to executable
#define DEFAULT_CONFIG_PATH "twp.conf"

// Helper to create a keybinding
KeyBinding create_binding(EditorAction action, int key) {
    KeyBinding binding;
    binding.action = action;
    binding.key = key;
    return binding;
}

// Initialize default configuration
Config* init_default_config() {
    Config* config = (Config*)malloc(sizeof(Config));
    if (!config) return NULL;
    
    // Set up normal mode bindings
    config->normal_count = 10;
    config->normal_bindings = (KeyBinding*)malloc(config->normal_count * sizeof(KeyBinding));
    if (!config->normal_bindings) {
        free(config);
        return NULL;
    }
    
    config->normal_bindings[0] = create_binding(ACTION_MOVE_LEFT, 'h');
    config->normal_bindings[1] = create_binding(ACTION_MOVE_RIGHT, 'l');
    config->normal_bindings[2] = create_binding(ACTION_MOVE_UP, 'k');
    config->normal_bindings[3] = create_binding(ACTION_MOVE_DOWN, 'j');
    config->normal_bindings[4] = create_binding(ACTION_PAGE_UP, 'u');
    config->normal_bindings[5] = create_binding(ACTION_PAGE_DOWN, 'd');
    config->normal_bindings[6] = create_binding(ACTION_GO_TO_TOP, 'g');
    config->normal_bindings[7] = create_binding(ACTION_GO_TO_BOTTOM, 'G');
    config->normal_bindings[8] = create_binding(ACTION_ENTER_INSERT_MODE, 'i');
    config->normal_bindings[9] = create_binding(ACTION_ENTER_COMMAND_MODE, ':');
    
    // Set up insert mode bindings
    config->insert_count = 6;
    config->insert_bindings = (KeyBinding*)malloc(config->insert_count * sizeof(KeyBinding));
    if (!config->insert_bindings) {
        free(config->normal_bindings);
        free(config);
        return NULL;
    }
    
    config->insert_bindings[0] = create_binding(ACTION_EXIT_INSERT_MODE, 27); // ESC key
    // Arrow keys (extended keys)
    config->insert_bindings[1] = create_binding(ACTION_MOVE_UP, 72);
    config->insert_bindings[2] = create_binding(ACTION_MOVE_DOWN, 80);
    config->insert_bindings[3] = create_binding(ACTION_MOVE_LEFT, 75);
    config->insert_bindings[4] = create_binding(ACTION_MOVE_RIGHT, 77);
    config->insert_bindings[5] = create_binding(ACTION_BACKSPACE, 8);
    
    // Set default config path
    strcpy(config->config_path, DEFAULT_CONFIG_PATH);
    
    return config;
}

// Parse a key string to a key code
int parse_key_string(const char* key_str) {
    if (strlen(key_str) == 1) {
        return key_str[0];
    } else if (strcmp(key_str, "ESC") == 0 || strcmp(key_str, "ESCAPE") == 0) {
        return 27;
    } else if (strcmp(key_str, "ENTER") == 0) {
        return 13;
    } else if (strcmp(key_str, "UP") == 0) {
        return 72;
    } else if (strcmp(key_str, "DOWN") == 0) {
        return 80;
    } else if (strcmp(key_str, "LEFT") == 0) {
        return 75;
    } else if (strcmp(key_str, "RIGHT") == 0) {
        return 77;
    } else if (strcmp(key_str, "BACKSPACE") == 0) {
        return 8;
    } else if (strcmp(key_str, "TAB") == 0) {
        return 9;
    }
    
    // Try to parse as integer (for direct key codes)
    char* end;
    long key_code = strtol(key_str, &end, 10);
    if (*end == '\0') {
        return (int)key_code;
    }
    
    return -1; // Invalid key string
}

// Convert action string to enum
EditorAction string_to_action(const char* action_str) {
    if (strcmp(action_str, "move_left") == 0) return ACTION_MOVE_LEFT;
    if (strcmp(action_str, "move_right") == 0) return ACTION_MOVE_RIGHT;
    if (strcmp(action_str, "move_up") == 0) return ACTION_MOVE_UP;
    if (strcmp(action_str, "move_down") == 0) return ACTION_MOVE_DOWN;
    if (strcmp(action_str, "page_up") == 0) return ACTION_PAGE_UP;
    if (strcmp(action_str, "page_down") == 0) return ACTION_PAGE_DOWN;
    if (strcmp(action_str, "go_to_top") == 0) return ACTION_GO_TO_TOP;
    if (strcmp(action_str, "go_to_bottom") == 0) return ACTION_GO_TO_BOTTOM;
    if (strcmp(action_str, "enter_insert_mode") == 0) return ACTION_ENTER_INSERT_MODE;
    if (strcmp(action_str, "exit_insert_mode") == 0) return ACTION_EXIT_INSERT_MODE;
    if (strcmp(action_str, "enter_command_mode") == 0) return ACTION_ENTER_COMMAND_MODE;
    if (strcmp(action_str, "backspace") == 0) return ACTION_BACKSPACE;
    if (strcmp(action_str, "enter_newline") == 0) return ACTION_ENTER_NEWLINE;
    if (strcmp(action_str, "save") == 0) return ACTION_SAVE;
    if (strcmp(action_str, "quit") == 0) return ACTION_QUIT;
    if (strcmp(action_str, "open") == 0) return ACTION_OPEN;
    if (strcmp(action_str, "new") == 0) return ACTION_NEW;
    
    return ACTION_NONE;
}

// Trim whitespace from string
char* trim(char* str) {
    char* end;
    
    // Trim leading spaces
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0)  // All spaces?
        return str;
    
    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end+1) = 0;
    
    return str;
}

// Load configuration from a file
Config* load_config(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        // If file doesn't exist, use default config
        Config* default_config = init_default_config();
        return default_config;
    }
    
    Config* config = init_default_config(); // Start with defaults
    if (!config) {
        fclose(file);
        return NULL;
    }
    
    // Copy the path
    strcpy(config->config_path, filepath);
    
    char line[256];
    STATE current_mode = normal;
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        
        // Skip comments and empty lines
        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }
        
        // Check for section headers
        if (strncmp(trimmed, "[normal]", 8) == 0) {
            current_mode = normal;
            continue;
        }
        if (strncmp(trimmed, "[insert]", 8) == 0) {
            current_mode = insert;
            continue;
        }
        
        // Parse key=value line
        char* eq_pos = strchr(trimmed, '=');
        if (eq_pos) {
            *eq_pos = '\0'; // Split the string
            char* action_str = trim(trimmed);
            char* key_str = trim(eq_pos + 1);
            
            EditorAction action = string_to_action(action_str);
            if (action == ACTION_NONE) {
                continue; // Invalid action
            }
            
            int key = parse_key_string(key_str);
            if (key < 0) {
                continue; // Invalid key
            }
            
            // Add to appropriate bindings array
            if (current_mode == normal) {
                // Check for existing binding first
                bool updated = false;
                for (int i = 0; i < config->normal_count; i++) {
                    if (config->normal_bindings[i].action == action) {
                        config->normal_bindings[i].key = key;
                        updated = true;
                        break;
                    }
                }
                
                if (!updated) {
                    // Expand the array
                    config->normal_count++;
                    config->normal_bindings = (KeyBinding*)realloc(
                        config->normal_bindings, 
                        config->normal_count * sizeof(KeyBinding)
                    );
                    config->normal_bindings[config->normal_count - 1] = create_binding(action, key);
                }
            } else { // insert mode
                // Similar to normal mode but for insert bindings
                bool updated = false;
                for (int i = 0; i < config->insert_count; i++) {
                    if (config->insert_bindings[i].action == action) {
                        config->insert_bindings[i].key = key;
                        updated = true;
                        break;
                    }
                }
                
                if (!updated) {
                    config->insert_count++;
                    config->insert_bindings = (KeyBinding*)realloc(
                        config->insert_bindings, 
                        config->insert_count * sizeof(KeyBinding)
                    );
                    config->insert_bindings[config->insert_count - 1] = create_binding(action, key);
                }
            }
        }
    }
    
    fclose(file);
    return config;
}

// Get key for a specific action
int get_key_for_action(Config* config, EditorAction action, STATE mode) {
    if (mode == normal) {
        for (int i = 0; i < config->normal_count; i++) {
            if (config->normal_bindings[i].action == action) {
                return config->normal_bindings[i].key;
            }
        }
    } else { // insert mode
        for (int i = 0; i < config->insert_count; i++) {
            if (config->insert_bindings[i].action == action) {
                return config->insert_bindings[i].key;
            }
        }
    }
    
    return -1; // Action not found
}

// Get action for a specific key
EditorAction get_action_for_key(Config* config, int key, STATE mode) {
    if (mode == normal) {
        for (int i = 0; i < config->normal_count; i++) {
            if (config->normal_bindings[i].key == key) {
                return config->normal_bindings[i].action;
            }
        }
    } else { // insert mode
        for (int i = 0; i < config->insert_count; i++) {
            if (config->insert_bindings[i].key == key) {
                return config->insert_bindings[i].action;
            }
        }
    }
    
    return ACTION_NONE; // Key not mapped
}

// Save configuration to file
bool save_config(Config* config) {
    FILE* file = fopen(config->config_path, "w");
    if (!file) {
        return false;
    }
    
    // Write header comment
    fprintf(file, "# TWP Editor Configuration\n");
    fprintf(file, "# Format: action=key\n\n");
    
    // Write normal mode bindings
    fprintf(file, "[normal]\n");
    for (int i = 0; i < config->normal_count; i++) {
        // Convert action enum to string
        char* action_str = "unknown";
        switch (config->normal_bindings[i].action) {
            case ACTION_MOVE_LEFT: action_str = "move_left"; break;
            case ACTION_MOVE_RIGHT: action_str = "move_right"; break;
            case ACTION_MOVE_UP: action_str = "move_up"; break;
            case ACTION_MOVE_DOWN: action_str = "move_down"; break;
            case ACTION_PAGE_UP: action_str = "page_up"; break;
            case ACTION_PAGE_DOWN: action_str = "page_down"; break;
            case ACTION_GO_TO_TOP: action_str = "go_to_top"; break;
            case ACTION_GO_TO_BOTTOM: action_str = "go_to_bottom"; break;
            case ACTION_ENTER_INSERT_MODE: action_str = "enter_insert_mode"; break;
            case ACTION_ENTER_COMMAND_MODE: action_str = "enter_command_mode"; break;
            default: continue; // Skip unknown actions
        }
        
        // For printing special keys nicely
        if (config->normal_bindings[i].key >= 32 && config->normal_bindings[i].key <= 126) {
            fprintf(file, "%s=%c\n", action_str, config->normal_bindings[i].key);
        } else {
            fprintf(file, "%s=%d\n", action_str, config->normal_bindings[i].key);
        }
    }
    
    // Write insert mode bindings
    fprintf(file, "\n[insert]\n");
    for (int i = 0; i < config->insert_count; i++) {
        // Convert action enum to string
        char* action_str = "unknown";
        switch (config->insert_bindings[i].action) {
            case ACTION_MOVE_LEFT: action_str = "move_left"; break;
            case ACTION_MOVE_RIGHT: action_str = "move_right"; break;
            case ACTION_MOVE_UP: action_str = "move_up"; break;
            case ACTION_MOVE_DOWN: action_str = "move_down"; break;
            case ACTION_EXIT_INSERT_MODE: action_str = "exit_insert_mode"; break;
            case ACTION_BACKSPACE: action_str = "backspace"; break;
            case ACTION_ENTER_NEWLINE: action_str = "enter_newline"; break;
            default: continue; // Skip unknown actions
        }
        
        // For printing special keys nicely
        if (config->insert_bindings[i].key >= 32 && config->insert_bindings[i].key <= 126) {
            fprintf(file, "%s=%c\n", action_str, config->insert_bindings[i].key);
        } else {
            switch (config->insert_bindings[i].key) {
                case 27: fprintf(file, "%s=ESC\n", action_str); break;
                case 13: fprintf(file, "%s=ENTER\n", action_str); break;
                case 8: fprintf(file, "%s=BACKSPACE\n", action_str); break;
                case 72: fprintf(file, "%s=UP\n", action_str); break;
                case 80: fprintf(file, "%s=DOWN\n", action_str); break;
                case 75: fprintf(file, "%s=LEFT\n", action_str); break;
                case 77: fprintf(file, "%s=RIGHT\n", action_str); break;
                default: fprintf(file, "%s=%d\n", action_str, config->insert_bindings[i].key);
            }
        }
    }
    
    fclose(file);
    return true;
}

// Free config resources
void free_config(Config* config) {
    if (config) {
        if (config->normal_bindings) {
            free(config->normal_bindings);
        }
        if (config->insert_bindings) {
            free(config->insert_bindings);
        }
        free(config);
    }
}
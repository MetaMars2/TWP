#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

typedef enum {
    ACTION_NONE,
    // Normal mode actions
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_MOVE_UP,
    ACTION_MOVE_DOWN,
    ACTION_PAGE_UP,
    ACTION_PAGE_DOWN,
    ACTION_GO_TO_TOP,
    ACTION_GO_TO_BOTTOM,
    ACTION_ENTER_INSERT_MODE,
    ACTION_ENTER_COMMAND_MODE,
    // Insert mode actions
    ACTION_EXIT_INSERT_MODE,
    ACTION_BACKSPACE,
    ACTION_ENTER_NEWLINE,
    // Command actions
    ACTION_SAVE,
    ACTION_QUIT,
    ACTION_OPEN,
    ACTION_NEW
} EditorAction;

typedef struct {
    EditorAction action;
    int key;
} KeyBinding;

typedef struct {
    KeyBinding* normal_bindings;
    int normal_count;
    KeyBinding* insert_bindings;
    int insert_count;
    char config_path[FILENAME_MAX];
} Config;

Config* init_default_config();
Config* load_config(const char* filepath);
bool save_config(Config* config);
int get_key_for_action(Config* config, EditorAction action, STATE mode);
EditorAction get_action_for_key(Config* config, int key, STATE mode);
void free_config(Config* config);

#endif // CONFIG_H
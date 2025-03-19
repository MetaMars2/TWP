#ifndef INPUT_H
#define INPUT_H

#include "common.h"
#include "editor.h"

bool process_input(EDITOR* editor);
void execute_command(EDITOR* editor, COMMANDS cmd);
bool state_normal_input(EDITOR* editor, char ch, bool input_changed);
void state_insert_input(EDITOR* editor, char ch, bool input_changed);
void state_command_input(EDITOR* editor, char ch, bool input_changed);

#endif // INPUT_H
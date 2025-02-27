#ifndef INPUT_H
#define INPUT_H

#include "common.h"
#include "editor.h"

bool process_input(EDITOR* editor);
void execute_command(EDITOR* editor, COMMANDS cmd);

#endif // INPUT_H
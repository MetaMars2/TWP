TWP Editor
TWP Editor is a lightweight, Vim-like text editor for the Windows terminal. It offers a simple yet effective interface for text editing with keyboard shortcuts inspired by Vim.

Features
Vim-inspired modal editing (Normal and Insert modes)
File operations (create, open, save)
Startup menu with ASCII art banner
Full file path support
Status bar showing editor state and filename
Smooth rendering using double-buffering system
Installation
Requirements
Windows operating system
GCC compiler
Make (optional, for using the Makefile)
Build Instructions
Clone or download the repository

Navigate to the project directory

Compile using the included Makefile:

Or manually compile with GCC:

Usage
Starting the Editor
Launch without arguments to show the startup menu:

Open a file directly:

Keyboard Controls
Normal Mode
h - Move cursor left
j - Move cursor down
k - Move cursor up
l - Move cursor right
i - Enter Insert mode
: - Enter command mode
Insert Mode
Arrow keys - Navigate
Type to insert text
Backspace - Delete character
Enter - Insert new line
Esc - Return to Normal mode
Commands
:w - Save file
:q - Quit
:o - Open a file
:new - Create new file

License
This project is released under the MIT License.
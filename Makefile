# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = 

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
EXECUTABLE = $(BIN_DIR)/twp.exe

# Default target
all: directories $(EXECUTABLE)

# Create necessary directories
directories:
	if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

# Link the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo Build complete: $(EXECUTABLE)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)
	@echo Cleaned build files

# Run the application
run: $(EXECUTABLE)
	$(EXECUTABLE)

# Install (copy to a user-accessible location)
install: $(EXECUTABLE)
	if not exist %USERPROFILE%\bin mkdir %USERPROFILE%\bin
	copy /Y $(EXECUTABLE) %USERPROFILE%\bin\twp.exe
	@echo Installed to %USERPROFILE%\bin\twp.exe

# Help target
help:
	@echo Available targets:
	@echo   all      - Build the executable (default)
	@echo   clean    - Remove build files
	@echo   run      - Build and run the application
	@echo   install  - Copy executable to user bin directory

.PHONY: all directories clean run install help
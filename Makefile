CC = gcc
CFLAGS = -Wall -Wextra -Wtype-limits -I./include
SRCS = src/main.c src/editor.c src/input.c src/display.c src/fileops.c src/ui.c src/cursor.c
OBJS = $(SRCS:.c=.o)
TARGET = twp

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
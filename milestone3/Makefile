CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = graph_gui
SRCS    = main.c graph.c gui.c
OBJS    = $(SRCS:.c=.o)

# raylib flags
RAYLIB_FLAGS = $(shell pkg-config --libs --cflags raylib 2>/dev/null || echo "-lraylib -lm -lpthread -ldl -lGL -lrt -lX11")
MATH_FLAG    = -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYLIB_FLAGS) $(MATH_FLAG)

%.o: %.c graph.h
	$(CC) $(CFLAGS) -c $< -o $@

gui.o: gui.c gui.h graph.h
	$(CC) $(CFLAGS) -c gui.c -o gui.o

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

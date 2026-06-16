CC      = gcc
CFLAGS  = -Wall -Wextra -g
RAYLIB  = $(shell pkg-config --libs --cflags raylib 2>/dev/null || echo "-lraylib -lm -lpthread -ldl -lGL -lrt -lX11")
MATH    = -lm

milestone1:
	$(CC) $(CFLAGS) -o milestone1/dijkstra milestone1/main.c milestone1/graph.c

milestone2:
	$(CC) $(CFLAGS) -o milestone2/sim milestone2/main.c milestone2/graph.c milestone2/gui.c $(RAYLIB) $(MATH)

milestone3:
	$(CC) $(CFLAGS) -o milestone3/sim milestone3/main.c milestone3/graph.c milestone3/gui.c $(RAYLIB) $(MATH)

milestone4:
	$(CC) $(CFLAGS) -o milestone4/sim milestone4/main.c milestone4/graph.c milestone4/gui.c $(RAYLIB) $(MATH)

milestone5:
	$(CC) $(CFLAGS) -o milestone5/sim milestone5/main.c milestone5/graph.c milestone5/gui.c $(RAYLIB) $(MATH)

clean:
	rm -f milestone1/dijkstra milestone1/*.o
	rm -f milestone2/sim milestone2/*.o
	rm -f milestone3/sim milestone3/*.o
	rm -f milestone4/sim milestone4/*.o
	rm -f milestone5/sim milestone5/*.o

.PHONY: milestone1 milestone2 milestone3 milestone4 milestone5 clean

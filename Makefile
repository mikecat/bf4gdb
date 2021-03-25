CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -O2

TARGET=bf4gdb
OBJS=main.o fileio.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)

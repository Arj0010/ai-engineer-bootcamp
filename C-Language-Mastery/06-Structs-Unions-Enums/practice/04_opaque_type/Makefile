# A minimal two-translation-unit build.
#
#   make          build ./stackdemo
#   make run      build and run
#   make clean
#
# Note that main.o depends on stack.h but NOT on stack.c. Change the struct
# layout in stack.c and only stack.o rebuilds — that is the payoff of an
# opaque type. Module 08 covers this properly.

CC      ?= gcc
CFLAGS  := -std=c17 -Wall -Wextra -Wpedantic -g -O2
TARGET  := stackdemo
OBJS    := main.o stack.o

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Both .o files depend on the header: touching stack.h rebuilds both.
main.o:  main.c  stack.h
stack.o: stack.c stack.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

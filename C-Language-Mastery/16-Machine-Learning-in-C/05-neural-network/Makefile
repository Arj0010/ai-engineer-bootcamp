CC     := cc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -O2
LDLIBS := -lm

.PHONY: all run clean
all: nn

nn: nn.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: nn
	./nn

clean:
	rm -f nn

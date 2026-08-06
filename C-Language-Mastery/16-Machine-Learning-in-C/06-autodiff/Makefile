CC     := cc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -O2
LDLIBS := -lm

.PHONY: all run clean
all: autodiff

autodiff: autodiff.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: autodiff
	./autodiff

clean:
	rm -f autodiff

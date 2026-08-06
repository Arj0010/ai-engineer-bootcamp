CC     := cc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -O2
LDLIBS := -lm

.PHONY: all run clean
all: linreg

linreg: linreg.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: linreg
	./linreg

clean:
	rm -f linreg

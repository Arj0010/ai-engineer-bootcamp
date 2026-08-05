# mathlib — a complete small C project build.
#
#   make            build build/mathdemo
#   make test       build and run the test suite
#   make static     build libmathlib.a and link a program against it
#   make shared     build libmathlib.so and link a program against it
#   make symbols    show what the linker sees (internal vs external linkage)
#   make deps       show the generated header dependencies
#   make clean
#
# EVERY RECIPE LINE MUST START WITH A TAB, not spaces. This is make's most
# notorious wart; the error you get is "missing separator".

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# NOTE: `?=` means "unless already set" — and make has a BUILT-IN default of
# CC=cc, so `CC ?= gcc` does nothing. Use `:=` to actually pin the compiler,
# or override on the command line: `make CC=clang`. This trips people up
# constantly, so it is spelled out rather than hidden.
CC       := cc
AR       := ar
CFLAGS   := -std=c17 -Wall -Wextra -Wpedantic -g -O2
CPPFLAGS := -Iinclude          # where #include "mathlib.h" is found
LDLIBS   :=

BUILD    := build

# ---------------------------------------------------------------------------
# Sources. LIB_SRCS excludes main.c, because the library must not contain a
# main() — otherwise you could never link it into a program that has one.
# ---------------------------------------------------------------------------
LIB_SRCS  := src/stats.c src/vector.c
LIB_OBJS  := $(LIB_SRCS:%.c=$(BUILD)/%.o)

MAIN_SRC  := src/main.c
MAIN_OBJ  := $(BUILD)/src/main.o

TEST_SRC  := tests/test_mathlib.c
TEST_OBJ  := $(BUILD)/tests/test_mathlib.o

TARGET    := $(BUILD)/mathdemo
TEST_BIN  := $(BUILD)/test_mathlib

# Every .o gets a matching .d file listing the headers it depends on.
DEPS      := $(LIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all test static shared symbols deps clean help

all: $(TARGET)

# ---------------------------------------------------------------------------
# Linking. $@ = the target, $^ = all prerequisites.
# ---------------------------------------------------------------------------
$(TARGET): $(LIB_OBJS) $(MAIN_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
	@echo "built $@"

# ---------------------------------------------------------------------------
# Compiling. $< = the first prerequisite.
#
# -MMD -MP is THE most important thing in this file. It makes the compiler
# emit build/src/stats.d containing:
#     build/src/stats.o: src/stats.c include/mathlib.h src/internal.h
# The `-include $(DEPS)` at the bottom pulls those rules in, so editing a
# HEADER rebuilds every file that includes it.
#
# Without this, `touch include/mathlib.h && make` does nothing, and you end up
# with object files compiled against different versions of a struct — which
# produces memory corruption, not a build error.
# ---------------------------------------------------------------------------
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

# ---------------------------------------------------------------------------
# Tests. A separate binary, because a program may have only one main().
# ---------------------------------------------------------------------------
$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: $(TEST_BIN)
	@echo "--------------------------------------------"
	@./$(TEST_BIN)

# ---------------------------------------------------------------------------
# A STATIC library: an archive of .o files. The linker copies the members you
# actually use INTO your executable. Bigger binary, no runtime dependency.
# ---------------------------------------------------------------------------
static: $(BUILD)/libmathlib.a $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $(BUILD)/mathdemo_static $(MAIN_OBJ) -L$(BUILD) -lmathlib
	@echo "--------------------------------------------"
	@ls -l $(BUILD)/libmathlib.a $(BUILD)/mathdemo_static
	@echo "note: -lmathlib comes AFTER the object file. The linker processes"
	@echo "arguments left to right and only pulls archive members that resolve"
	@echo "symbols it has ALREADY seen. Put -l flags last."

$(BUILD)/libmathlib.a: $(LIB_OBJS)
	$(AR) rcs $@ $^
	@echo "created $@"

# ---------------------------------------------------------------------------
# A SHARED library: loaded at run time. -fPIC is required because the code
# must work at whatever address the loader picks.
# ---------------------------------------------------------------------------
shared: $(BUILD)/libmathlib.so $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $(BUILD)/mathdemo_shared $(MAIN_OBJ) -L$(BUILD) -lmathlib
	@echo "--------------------------------------------"
	@ls -l $(BUILD)/libmathlib.so $(BUILD)/mathdemo_shared
	@echo "the shared binary is SMALLER — the library is not copied in."
	@echo "it also will not run without the .so being findable:"
	@LD_LIBRARY_PATH=$(BUILD) ./$(BUILD)/mathdemo_shared >/dev/null && \
	  echo "  LD_LIBRARY_PATH=$(BUILD) ./$(BUILD)/mathdemo_shared  -> ok"

$(BUILD)/libmathlib.so: $(LIB_SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared -o $@ $^
	@echo "created $@"

# ---------------------------------------------------------------------------
# What the linker actually sees.
# ---------------------------------------------------------------------------
symbols: $(LIB_OBJS) $(MAIN_OBJ)
	@echo "=== stats.o ==="
	@nm --defined-only $(BUILD)/src/stats.o | grep -v '\.LC' | sed 's/^/  /'
	@echo "  T = external linkage (in the Text section, visible to the linker)"
	@echo "  t = INTERNAL linkage (static) — invisible outside this file"
	@echo "  D = initialised data with external linkage"
	@echo
	@echo "=== vector.o: symbols it NEEDS from elsewhere ==="
	@nm --undefined-only $(BUILD)/src/vector.o | sed 's/^/  /'
	@echo "  U = Undefined. ml_sqrt_newton lives in stats.o; the LINKER"
	@echo "      matches them. If nothing defines it you get the classic"
	@echo "      'undefined reference' error."
	@echo
	@echo "=== main.o: symbols it NEEDS ==="
	@nm --undefined-only $(BUILD)/src/main.o | sed 's/^/  /'
	@echo
	@echo "note: cmp_double and clamp01 are 'static' in stats.c, so they show"
	@echo "as lowercase t (or not at all) and cannot collide with a same-named"
	@echo "function in another file. THIS is why you mark everything static."

deps: $(LIB_OBJS)
	@echo "generated by -MMD -MP:"
	@cat $(BUILD)/src/stats.d | sed 's/^/  /'
	@echo
	@echo "Try it:"
	@echo "  touch include/mathlib.h && make   -> rebuilds ALL THREE .o files"
	@echo "  touch src/internal.h   && make   -> rebuilds only stats.o and"
	@echo "                                      vector.o (main.c never sees it)"
	@echo "  touch src/vector.c     && make   -> rebuilds only vector.o"
	@echo "That precision is what -MMD buys you. Without it, editing a header"
	@echo "rebuilds NOTHING, and you silently link .o files that disagree about"
	@echo "a struct layout — memory corruption, with no build error."

clean:
	rm -rf $(BUILD)
	@echo "cleaned"

help:
	@sed -n '3,12p' Makefile

# Pull in the auto-generated header dependencies. The leading '-' means
# "do not fail if they do not exist yet" — on the first build they do not.
-include $(DEPS)

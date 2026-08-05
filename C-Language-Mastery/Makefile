# C Language Mastery — top-level build
#
#   make              build every standalone program into build/
#   make M=04-Pointers  build only that module
#   make run FILE=path/to/file.c   compile and run one file
#   make asan         rebuild everything with AddressSanitizer + UBSan
#   make clean        delete build/
#
# Multi-file projects live in directories that contain their own Makefile;
# they are skipped here. cd into them and run `make`.

CC      ?= gcc
CSTD    ?= -std=c17
WARN    := -Wall -Wextra -Wpedantic
OPT     ?= -O2
CFLAGS  := $(CSTD) $(WARN) -g $(OPT)
LDLIBS  := -lm -lpthread

BUILD   := build
M       ?=

# Every .c file that (a) defines main() and (b) is not part of a directory
# that manages itself with its own Makefile.
SRCS := $(shell ./list-programs.sh $(if $(M),./$(M),.))

# ./05-Memory-Management/practice/03_arena.c -> build/05-Memory-Management_practice_03_arena
BINS := $(foreach s,$(SRCS),$(BUILD)/$(subst /,_,$(patsubst ./%,%,$(basename $(s)))))

.PHONY: all clean asan run list projects help

all: $(BINS)
	@echo "built $(words $(BINS)) programs into $(BUILD)/"

$(BUILD):
	@mkdir -p $(BUILD)

# One pattern cannot express the path mangling, so generate a rule per source.
define COMPILE_RULE
$(BUILD)/$(subst /,_,$(patsubst ./%,%,$(basename $(1)))): $(1) | $(BUILD)
	@$(CC) $(CFLAGS) $$< -o $$@ $(LDLIBS) && echo "  CC  $(1)"
endef
$(foreach s,$(SRCS),$(eval $(call COMPILE_RULE,$(s))))

# Rebuild everything with the sanitizers on. Slower, but it turns silent
# memory corruption into a loud stack trace.
asan:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory OPT="-O1 -fno-omit-frame-pointer -fsanitize=address,undefined"

# make run FILE=01-Fundamentals/practice/01_types_and_sizes.c
run:
	@test -n "$(FILE)" || { echo "usage: make run FILE=path/to/file.c"; exit 1; }
	@$(CC) $(CFLAGS) $(FILE) -o $(BUILD)/_run $(LDLIBS) && $(BUILD)/_run
	@:

list:
	@printf '%s\n' $(SRCS)

# Build every self-managed multi-file project.
projects:
	@find . -name Makefile -not -path './Makefile' -not -path './$(BUILD)/*' | \
	  while read -r mk; do \
	    d=$$(dirname "$$mk"); \
	    echo "==> $$d"; \
	    $(MAKE) --no-print-directory -C "$$d" || exit 1; \
	  done

clean:
	@rm -rf $(BUILD)
	@find . -name Makefile -not -path './Makefile' | while read -r mk; do \
	   $(MAKE) --no-print-directory -C "$$(dirname $$mk)" clean >/dev/null 2>&1 || true; done
	@echo "cleaned"

help:
	@sed -n '1,12p' Makefile

# Nicolai Brand (lytix.dev), Callum Gran 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d)
SRCS := $(shell find $(SRC) -type f -name "*.c")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wpedantic -Wextra -Wshadow -std=c11
CFLAGS += -DNDEBUG
LDFLAGS = -lm

.PHONY: format clean tags bear $(OBJDIR)
TARGET = slash
TARGET-FUZZ = slash-fuzz

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(TARGET-FUZZ): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

debug: CFLAGS = -Iinclude -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g -DDEBUG -DDEBUG_PERF
debug: $(TARGET)

fuzz: CFLAGS += -fsanitize=address -fsanitize=undefined
fuzz: LDFLAGS += -fsanitize=address -fsanitize=undefined
fuzz: $(TARGET-FUZZ)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET-FUZZ)

tags:
	@ctags -R

bear:
	@bear -- make

format:
	./format.slash # you will need slash installed to do this

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))

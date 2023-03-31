# Nicolai Brand (lytix.dev) 2022-2023
# See LICENSE for license info

RC = .slashrc
OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d)
SRCS := $(shell find $(SRC) -type f -name "*.c")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CC = gcc
CFLAGS = -I include -Wall -Wpedantic -Wextra -Wshadow -std=c11

.PHONY: clean tags bear $(OBJDIR)
TARGET = slash

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@cp $(RC) ~
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

debug-verbose: CFLAGS += -DDEBUG_VERBOSE
debug-verbose: debug

clean:
	@rm -rf $(OBJDIR) $(TARGET) ~/$(RC)

tags:
	@ctags -R

bear:
	bear -- make

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))

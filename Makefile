# Nicolai Brand (lytix.dev), Callum Gran 2022-2023
# See LICENSE for license info

RC = .slashrc
OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d)
SRCS := $(shell find $(SRC) -type f -name "*.c")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wpedantic -Wextra -Wshadow -std=c11
CFLAGS += -DNDEBUG

.PHONY: format clean tags bear $(OBJDIR)
TARGET = slash

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@cp $(RC) ~
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET_CLIENT) $(TARGET_GUI) $(TARGET_GUI_MACOS)

tags:
	@ctags -R

bear:
	@bear -- make

format:
	python format.py

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))

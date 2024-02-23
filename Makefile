# Nicolai Brand (lytix.dev), Callum Gran 2022-2023
# See LICENSE for license info

OBJDIR = .obj
SRC = src
DIRS := $(shell find $(SRC) -type d)
SRCS := $(shell find $(SRC) -type f -name "*.c")
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

CFLAGS = -Iinclude -Wall -Wpedantic -Wextra -Wshadow -std=gnu11 -O2
CFLAGS += -DNDEBUG
LDFLAGS = -lm

.PHONY: format clean tags bear $(OBJDIR)
TARGET = slash
TARGET-ASAN = slash-asan

all: $(TARGET)

$(OBJDIR)/%.o: %.c Makefile | $(OBJDIR)
	@echo [CC] $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@echo [LD] $@
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(TARGET-ASAN): $(OBJS)
	@echo [LD] $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

debug: CFLAGS = -Iinclude -Wall -Wpedantic -Wextra -Wshadow -std=gnu11 -g -DDEBUG -DDEBUG_PERF
debug: $(TARGET)

asan: CFLAGS += -fsanitize=address -fsanitize=undefined -DDEBUG_STRESS_GC
asan: LDFLAGS += -fsanitize=address -fsanitize=undefined
asan: $(TARGET-ASAN)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET-ASAN)

tags:
	@ctags -R

bear:
	@bear -- make

format:
	./format.slash # you will need slash installed to do this

tidy:
	clang-tidy $(SRCS) -- $(CFLAGS)

install: $(TARGER)
	@echo [INSTALL] $(TARGET) to /usr/local/bin
	@sudo cp $(TARGET) /usr/local/bin
	

$(OBJDIR):
	$(foreach dir, $(DIRS), $(shell mkdir -p $(OBJDIR)/$(dir)))

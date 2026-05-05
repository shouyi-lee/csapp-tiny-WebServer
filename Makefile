CC ?= gcc
TARGET ?= webserver
TARGET_PATH := $(TARGET)
LOG_DIR := log_file
RUN_PORT ?= 1145
RUN_COND ?= 
BASIC_HTML ?= website/index.html

SRCS := $(shell find src -type f -name '*.c' | sort)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

CPPFLAGS ?=
CPPFLAGS += -Isrc/include
CFLAGS ?= -Wall -Wextra -O2 -std=gnu11
DEPFLAGS := -MMD -MP
LDFLAGS ?=
LDLIBS ?=
RM ?= rm -f

.PHONY: all clean rebuild run

all: $(TARGET_PATH)


$(TARGET_PATH): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#	$(RM) $(OBJS) $(DEPS)

$(LOG_DIR):
	mkdir -p log_file

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET_PATH) | $(LOG_DIR) $(BASIC_HTML)
	./$(TARGET) $(RUN_PORT) $(RUN_COND)

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET_PATH)

rebuild: clean all

-include $(DEPS)
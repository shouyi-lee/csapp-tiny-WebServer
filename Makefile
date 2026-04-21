CC ?= gcc
TARGET ?= webserver
TARGET_PATH := $(TARGET)
LOG_DIR := log
RUN_PORT ?= 1145
RUN_COND ?= &

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

CPPFLAGS ?=
CFLAGS ?= -Wall -Wextra -g -O0 -std=gnu11
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
	mkdir -p log

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET_PATH) | $(LOG_DIR)
	./$(TARGET) $(RUN_PORT) $(RUN_COND)

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET_PATH)

rebuild: clean all

-include $(DEPS)
CC ?= gcc
TARGET ?= webserver
TARGET_PATH := $(TARGET)
DEBUG ?= f
CONFIG ?= server.conf

SRCS := $(shell find src -type f -name '*.c' | sort)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

CPPFLAGS ?=
CPPFLAGS += -Isrc/include

CFLAGS ?= -Wall -Wextra -std=gnu11
ifeq ($(DEBUG), f)
CFLAGS += -O2
else
CFLAGS += -g
endif

DEPFLAGS := -MMD -MP
LDFLAGS ?=
LDLIBS ?=
RM ?= rm -f

.PHONY: all clean rebuild run

all: $(TARGET_PATH)

$(TARGET_PATH): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#	$(RM) $(OBJS) $(DEPS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET_PATH)
	./$(TARGET) $(CONFIG)

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET_PATH)

rebuild: clean all

-include $(DEPS)
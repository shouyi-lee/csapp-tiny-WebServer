CC ?= gcc
TARGET ?= webserver
OUTDIR ?= webServer
TARGET_PATH := $(OUTDIR)/$(TARGET)
RUN_PORT ?= 1144

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


$(TARGET_PATH): $(OBJS) | $(OUTDIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#	$(RM) $(OBJS) $(DEPS)

$(OUTDIR):
	mkdir -p $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET_PATH)
	cd $(OUTDIR) && ./$(TARGET) $(RUN_PORT)

clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET_PATH)
	rmdir --ignore-fail-on-non-empty $(OUTDIR) 2>/dev/null || true

rebuild: clean all

-include $(DEPS)
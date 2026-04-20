CC      := gcc
CFLAGS  := -I../src/ -Wall -Wextra -O2
TARGET  ?= basicNetworkingExample

SRC := $(wildcard ../src/*.c)
SRC += $(TARGET).c

ifeq ($(OS),Windows_NT)
    SRC += ../src/platform/windows.c
    LDFLAGS += -lws2_32
else
    SRC += ../src/platform/linux.c
endif

OBJS := $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

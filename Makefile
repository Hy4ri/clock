.PHONY: all clean

CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -O2 $(shell pkg-config --cflags x11 xft fontconfig)
LDFLAGS = $(shell pkg-config --libs x11 xft fontconfig)

SRCS    = main.c clock.c
OBJS    = $(SRCS:.c=.o)
TARGET  = clock

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

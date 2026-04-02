CC=gcc
CFLAGS=-Wall -Wextra -O2 -std=c11 -I/opt/homebrew/opt/cjson/include
TARGET=speedtest
SRCS=main.c json_parser.c geo.c server_select.c speedtest.c
OBJS=$(SRCS:.c=.o)
LIBS=-L/opt/homebrew/opt/cjson/lib -lcurl -lcjson

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
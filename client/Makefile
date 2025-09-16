CC=gcc
CFLAGS=-Wall -Wextra -O2
BIN=client

all: $(BIN)

$(BIN): client.c
	$(CC) $(CFLAGS) -o $(BIN) client.c

clean:
	rm -f $(BIN)

CC=gcc
CFLAGS=-Wall -Wextra -O2
BIN=client
SRC=src/client.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -f $(BIN)

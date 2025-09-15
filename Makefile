CC=gcc
CFLAGS=-O2 -Wall -Wextra -pthread -Iinclude
LDFLAGS=-pthread -lm

BIN=imageserver
SRC=src/imageserver.c src/config.c src/logging.c src/jobs.c src/process.c src/net.c
OBJ=$(SRC:.c=.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

src/%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -Dm755 $(BIN) /usr/local/bin/$(BIN)
	install -Dm644 config.conf /etc/server/config.conf
	install -d /var/lib/imageserver /var/lib/imageserver/tmp
	install -Dm644 /dev/null /var/log/imageserver.log || true
	chmod 644 /var/log/imageserver.log

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all install clean

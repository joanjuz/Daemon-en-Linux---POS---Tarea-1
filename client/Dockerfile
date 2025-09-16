FROM alpine:3.20
RUN apk add --no-cache build-base
WORKDIR /app
COPY src/client.c .
RUN gcc -O2 -Wall -Wextra -o client client.c
ENTRYPOINT ["./client"]

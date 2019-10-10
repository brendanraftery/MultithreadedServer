# If you add .c/.h pairs, add their names without any extension here
# Try to only modify this line
SOURCE_NAMES = main server misc http_messages

# Use GNU compiler
CC = gcc -g -Wall -Werror -pthread

SRC_H = $(SOURCE_NAMES:=.h) socket.h tcp.h tls.h
SRC_O = $(SOURCE_NAMES:=.o)

all: git myhttpd myhttpsd

tcp_socket.o: socket.c $(SRC_H)
	$(CC) -c $< -o $@

tcp.o: tcp.c $(SRC_H)
	$(CC) -c $< -o $@

tls_socket.o: socket.c $(SRC_H)
	$(CC) -D USE_TLS -c $< -o $@

tls.o: tls.c $(SRC_H)
	$(CC) `pkg-config --cflags openssl` -c $< -o $@

$(SRC_O) : %.o : %.c $(SRC_H)
	$(CC) -c $<

myhttpd: $(SRC_O) tcp_socket.o tcp.o
	$(CC) -o $@ $^

myhttpsd: $(SRC_O) tls_socket.o tls.o
	$(CC) -o $@ $^ `pkg-config --libs openssl`

.PHONY: git
# DO NOT MODIFY
git:
	git add *.c *.h Makefile >> .local.git.out || echo
	git commit -a -m "Commit lab 5" >> .local.git.out || echo
	git push origin HEAD:master

.PHONY: clean
clean:
	rm -f myhttpd myhttpsd tcp_socket.o tls_socket.o tcp.o tls.o $(SRC_O)

#include "socket.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Read a line from the socket. On error, return NULL. It is the caller's
 * responsibility to free() the returned string.
 */

char *socket_read_line(socket_t *socket) {
  // The minimum size allocated for the line, not including
  // the terminating null character

#define INIT_LINE_SIZE (16)

  int max_length = INIT_LINE_SIZE;
  char *line = malloc(sizeof(char) * max_length + 1);

  int i = 0;

  // continue until we see \r\n

  while (i < 2 || line[i - 2] != '\r' || line[i - 1] != '\n') {
    char c = socket_getc(socket);

    if (c == EOF) {
      break;
    }
    else if (c < 0) {
      // indicates some other error

      free(line);
      return NULL;
    }

    if (i >= max_length) {
      max_length *= 2;
      line = realloc(line, sizeof(char) * max_length + 1);
    }

    line[i++] = c;
  }
  line[i] = '\0';
  return line;
}

/*
 * Polymorphically get a character from the socket.  Return EOF for EOF and some
 * other negative value for any other error.
 */

char socket_getc(socket_t *socket) {
#ifdef USE_TLS
  return tls_getc(socket);
#else
  return tcp_getc(socket);
#endif
}

/*
 * Write a null-terminated string to the socket. Return 0 on success.
 */

int socket_write_string(socket_t *socket, char *string) {
  return socket_write(socket, string, strlen(string));
}

/*
 * Polymorphically write a number of characters specified by length from buffer
 * to socket. Return 0 on success.
 */

int socket_write(socket_t *socket, char *buffer, size_t length) {
#ifdef USE_TLS
  return tls_write(socket, buffer, length);
#else
  return tcp_write(socket, buffer, length);
#endif
}

/*
 * Polymorphically close and free the socket. Return 0 on success. The caller
 * should set socket to NULL afterwards to avoid referencing a socket which will
 * be invalid.
 */

int close_socket(socket_t *socket) {
#ifdef USE_TLS
  return close_tls_socket(socket);
#else
  return close_tcp_socket(socket);
#endif
}

/*
 * Polymorphically create a new socket acceptor, taking the port number as an
 * argument. Return NULL on error. It is the caller's responsibility to call
 * close_socket_acceptor() to close and free the acceptor.
 */

acceptor *create_socket_acceptor(int port) {
#ifdef USE_TLS
  return create_tls_acceptor(port);
#else
  return create_tcp_acceptor(port);
#endif
}

/*
 * Polymorphically accept a new connection on the passed socket acceptor. Return
 * NULL on error.
 */

socket_t *accept_connection(acceptor *socket_acceptor) {
#ifdef USE_TLS
  return accept_tls_connection(socket_acceptor);
#else
  return accept_tcp_connection(socket_acceptor);
#endif
}

/*
 * Polymorphically close and free the passed socket acceptor. Return 0 on
 * success. The caller should set socket_acceptor to NULL afterwards to avoid
 * referencing an acceptor which will be invalid.
 */

int close_socket_acceptor(acceptor* socket_acceptor) {
#ifdef USE_TLS
  return close_tls_acceptor(socket_acceptor);
#else
  return close_tcp_acceptor(socket_acceptor);
#endif
}

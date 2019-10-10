#include "tls.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Close and free a TLS socket created by accept_tls_connection(). Return 0 on
 * success. You should use the polymorphic version of this function, which is
 * close_socket() in socket.c.
 */

int close_tls_socket(tls_socket *socket) {
  // TODO: Add your code to close the socket
  printf("Closing TLS socket fd %d", socket->socket_fd);

  char inet_pres[INET_ADDRSTRLEN];

  if (inet_ntop(socket->addr.sin_family, &(socket->addr.sin_addr), inet_pres, INET_ADDRSTRLEN)) {
    printf(" from %s", inet_pres);
  }

  putchar('\n');

  int status = close(socket->socket_fd);
  free(socket);

  return status;
}

/*
 * Get a character from the TLS socket. You should use the polymorphic version
 * of this function, which is socket_getc() in socket.c.
 */

char tls_getc(tls_socket *socket) {
  // TODO: Add your code to get a character from the socket
  
#define RECV_FAILED (-1)

  char c;
  ssize_t read = recv(socket->socket_fd, &c, 1, 0);
  if (read < 0 || read > 1) {
    return RECV_FAILED;
  }

  if (read == 0) {
    return EOF;
  }

  return c;
}

/*
 * Write a buffer of length buf_len to the TLS socket. Return 0 on success. You
 * should use the polymorphic version of this function, which is socket_write()
 * in socket.c
 */

int tls_write(tls_socket *socket, char *buf, size_t buf_len) {
  if (buf == NULL) {
    return -1;
  }

  // TODO: Add your code to write to the socket
  size_t sent = send(socket->socket_fd, buf, buf_len, 0);
  if (sent < 0) {
    return sent;
  }
  else if (sent != buf_len) {
    size_t i;
    char buf_in_hex[(buf_len * 2) + 1];
    for (i = 0; i < buf_len; i++) {
      char current_hex[2 + 1];
      snprintf(current_hex, 2 + 1, "%x", buf[i]);
      strncat(buf_in_hex, current_hex, 2);
    }

    fprintf(stderr, "Could not write all bytes of : '%s'."
                    "Expected %ld but actually sent %ld\n",
                    buf_in_hex, buf_len, sent);
    return -1;
  }
  return 0;
}

/*
 * Create a new TLS socket acceptor, listening on the given port. Return NULL on
 * error. You should ues the polymorphic version of this function, which is
 * create_socket_acceptor() in socket.c.
 */

tls_acceptor *create_tls_acceptor(int port) {
  tls_acceptor *acceptor = malloc(sizeof(tls_acceptor));

  acceptor->addr.sin_family = AF_INET;
  acceptor->addr.sin_port = htons(port);
  acceptor->addr.sin_addr.s_addr = htonl(INADDR_ANY);

  acceptor->master_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (acceptor->master_socket < 0) {
    fprintf(stderr, "Unable to create socket: %s\n", strerror(errno));
    return NULL;
  }

  int optval = 1;
  if (setsockopt(acceptor->master_socket,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 &optval,
                 sizeof(optval)) < 0) {
    fprintf(stderr, "Unable to set socket options: %s\n", strerror(errno));
  }
  if (bind(acceptor->master_socket,
           (struct sockaddr*) &acceptor->addr,
           sizeof(acceptor->addr)) < 0) {
    fprintf(stderr, "Unable to bind to socket: %s\n", strerror(errno));
  }

  if (listen(acceptor->master_socket, 50) < 0) {
    fprintf(stderr, "Unable to listen to socket: %s\n", strerror(errno));
  }

  return acceptor;
}

/*
 * Accept a new connection from the TLS socket acceptor. Return NULL on error,
 * and the new TLS socket otherwise. You should use the polymorphic version of
 * this function, which is accept_connection() in socket.c.
 */

tls_socket *accept_tls_connection(tls_acceptor *acceptor) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  int socket_fd = accept(acceptor->master_socket,
                         (struct sockaddr*) &addr,
                         &addr_len);
  if (socket_fd == -1) {
    fprintf(stderr, "Unable to accept connection: %s\n", strerror(errno));
    return NULL;
  }

  tls_socket *socket = malloc(sizeof(tls_socket));
  socket->socket_fd = socket_fd;
  socket->addr = addr;

  char inet_pres[INET_ADDRSTRLEN];
  if (inet_ntop(addr.sin_family,
                &(addr.sin_addr),
                inet_pres,
                INET_ADDRSTRLEN)) {
    printf("Received a connection from %s\n", inet_pres);
  }

  return socket;
}

/*
 * Close and free the passed TLS socket acceptor. Return 0 on success. You
 * should use the polymorphic version of this function, which is
 * close_socket_acceptor() in socket.c.
 */

int close_tls_acceptor(tls_acceptor *acceptor) {
  // TODO: Add your code to close the master socket
  printf("Closing socket %d\n", acceptor->master_socket);
  int status = close(acceptor->master_socket);
  free(acceptor);
  return status;
}

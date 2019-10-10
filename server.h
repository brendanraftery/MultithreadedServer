#ifndef SERVER_H
#define SERVER_H

#include "socket.h"

void run_linear_server(acceptor *acceptor);
void run_forking_server(acceptor *acceptor);
void run_threaded_server(acceptor *acceptor);
void run_thread_pool_server(acceptor *acceptor, int num_threads);

void handle(socket_t *sock);

#endif // SERVER_H

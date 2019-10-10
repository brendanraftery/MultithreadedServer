#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>

#include "http_messages.h"

pthread_mutex_t pool_lock;

void loop_thread(acceptor *acceptor);
void parse_request(socket_t *sock, http_request * request);
char *map_path(char *original);
int ends_with(char *file, char *ending);

/*
 * Accept connections one at a time and handle them.
 */

void run_linear_server(acceptor *acceptor) {
  while (1) {
    socket_t *sock = accept_connection(acceptor);
    handle(sock);
  }
}

/*
 * Accept connections, creating a different child process to handle each one.
 */

void run_forking_server(acceptor *acceptor) {
  // TODO: Add your code to accept and handle connections in child processes
  while (1) {
    socket_t *sock = accept_connection(acceptor);
    int ret = fork();
    if (ret == 0) {
      handle(sock);
      exit(EXIT_SUCCESS);
    } else {
      wait(NULL);
    }
    close_socket(sock);
  }
}

/*
 * Accept connections, creating a new thread to handle each one.
 */

void run_threaded_server(acceptor *acceptor) {
  // TODO: Add your code to accept and handle connections in new threads
  while (1) {
    socket_t *sock = accept_connection(acceptor);
    pthread_t thread;
    pthread_create(&thread, NULL, (void *(*)(void *)) handle, (void *) sock);
    pthread_detach(thread); 
  }
}

/*
 * Accept connections, drawing from a thread pool with num_threads to handle the
 * connections.
 */

void run_thread_pool_server(acceptor *acceptor, int num_threads) {
  // TODO: Add your code to accept and handle connections in threads from a
  // thread pool

  pthread_mutex_init(&pool_lock, NULL);

  printf("Creating %d pool\n", num_threads);

  while (1) {
    int i = 0;
    pthread_t pool[num_threads];
    for (i = 0; i < num_threads; i++) {
      pthread_create(&pool[i], NULL, (void *(*)(void *)) loop_thread, (void *) acceptor);
     // pthread_detach(pool[i]);
    }
    pthread_join(pool[0], NULL);
  }
}

/*
 * Works as a helper function of run_thread_pool_server
 */

void loop_thread(acceptor *acceptor) {
  while (1) {
    pthread_mutex_lock(&pool_lock);
    socket_t *sock = accept_connection(acceptor);
    handle(sock);
    pthread_mutex_unlock(&pool_lock);
  }
} /* loop_thread() */

/*
 * Handle an incoming connection on the passed socket.
 */

void handle(socket_t *sock) {
  http_request request;

  // TODO: Replace this code and actually parse the HTTP request

  request.method = "GET";
  request.request_uri = "/";
  request.query = "";
  request.http_version = "HTTP/1.1";
  request.num_headers = 0;
  request.message_body = "";

  parse_request(sock, &request);

  http_response response;

  // TODO: Add your code to create the correct HTTP response

  printf("BEGINNING WRITE TO SOCKET\n********************************\n");
  response.http_version = request.http_version;

  // Validate and path request_uri
  char *path = map_path(request.request_uri);

  printf("FINAL PATH : %s\n", path);

  struct stat path_stat;
  stat(path, &path_stat);

/*
  int file_num = 0;
  if (S_ISREG(path_stat.st_mode)) {
    request.data = 7;
    char *command = "ls -l ";
    strcat(command, path);
    file_num = system(command);

    file_link *files = (file_link*)malloc(sizeof(file_link)*file_num);
  }
*/

  if (access(path, F_OK) == -1) {
    // path invalid
    response.status_code = 404;
  }
  else {
    response.status_code = 200;
  }

  if (request.data == 401) {
    response.status_code = 401;
  }

  int is_image = 0;

  // Determine content type
  if ((ends_with(path, ".html") == 0) || (ends_with(path, ".html/") == 0)) {
    response.content_type = "text/html";
  }
  else if ((ends_with(path, ".gif") == 0) || (ends_with(path, ".gif/") == 0)) {
    response.content_type = "image/gif";
  }
  else if ((ends_with(path, ".ico") == 0) || (ends_with(path, ".ico/") == 0)) {
    response.content_type = "image/x-icon";
  }
  else if ((ends_with(path, ".png") == 0) || (ends_with(path, ".png/") == 0)) {
    response.content_type = "image/png";
  }
  else if ((ends_with(path, ".jpg") == 0) || (ends_with(path, ".jpg/") == 0)) {
    response.content_type = "image/jpg";
  }
  else if ((ends_with(path, ".svg") == 0) || (ends_with(path, ".svg/") == 0)) {
    response.content_type = "image/svg+xml";
  }
  else if ((ends_with(path, ".xbm") == 0) || (ends_with(path, ".xbm/") == 0)) {
    response.content_type = "image/xbm";
  }
  else {
    response.content_type = "text/plain";
  }

  // Write header
  char *to_string = response_string(&response);
  printf("%s\n", to_string);
  socket_write_string(sock, to_string);

  struct stat st;
  stat(path, &st);
  int file_size = st.st_size;

  // Write data (Might put open above header for checking purposes)
  if (response.status_code == 200 && request.data != 7) {

    int c = 0;
    FILE *fd = fopen(path, "r");
    if (fd) {

      if (is_image == 1) {
        while ((c = getc(fd)) != EOF) {
          char buf[1];
          printf("%c", c);
          buf[0] = c;
          socket_write(sock, buf, 1);
        }
      }
      else {
        char buffer[file_size + 1];
        fread(buffer, sizeof(char), file_size, fd);
        socket_write(sock, buffer, file_size);
      }

      fclose(fd);
    }
  }

  free(to_string);
  to_string = NULL;
  printf("********************************\nEND WRITE TO SOCKET\n");
  close_socket(sock);
} /* handle() */

char *map_path(char *original) {
  char cwd[256];
  getcwd(cwd, 255);

  char head[20];
  strncpy(head, original, 6);
  // check for icons
  if (strcmp(head, "/icons") == 0) {
    strcat(cwd, "/http-root-dir");
    strcat(cwd, original);
    return strdup(cwd);
  } 

  strncpy(head, original, 7);
  // check for htdocs
  if (strcmp(head, "/htdocs") == 0) {
    strcat(cwd, "/http-root-dir");
    strcat(cwd, original);
    return strdup(cwd);
  }

  // check for "/"
  if (original[0] == '/' && original[1] == '\0') {
    strcat(cwd, "/http-root-dir/htdocs/index.html");
    return strdup(cwd);
  }

  // other
  strcat(cwd, "/http-root-dir/htdocs");
  strcat(cwd, original);

  return strdup(cwd);
}

void parse_request(socket_t *sock, http_request *request) {
  char new_char;
  char old_char;
  int length = 0;
  int got_get = 0;
  int got_http = 0;
  int got_doc = 0;
  char *doc_path;
  char get_check[1024];
  char *http_version;
  char curr_string[1024];

  new_char = socket_getc(sock);

  // CHECK FOR GET AND PATH

  while (new_char != EOF) {
    length++;

    if (new_char == ' ') { // Space detected
      if ((got_get == 0) && (strcmp(get_check, "GET") == 0)) {
        length = 0;
        got_get = 1;
      }
      else if (got_get == 1 && got_doc == 0) {
        got_doc = 1;
        curr_string[length-1] = '\0';
        doc_path = (char *)malloc(sizeof(char) * length);
        strcpy(doc_path, curr_string);
        length = 0;
      }
      else if (got_get == 1 && got_doc == 1 && got_http == 0) {
        got_http = 1;
        curr_string[length-1] = '\0';
        http_version = (char *)malloc(sizeof(char) * length);
        strcpy(http_version, curr_string);
        length = 0;
      }
    }
    else if ((new_char == '\n') && (old_char == '\r')) {
      break;
    }
    else if (got_get == 1) {
      old_char = new_char;
      curr_string[length - 1] = new_char;
    }
    else if (got_get == 0) {
      old_char = new_char;
      get_check[length-1] = new_char;
      get_check[length] = '\0';
    }

    new_char = socket_getc(sock);
  } 

  // CHECK FOR AUTHORIZATION
  char *auth = "default";
  request->data = 401;


  while (strcmp(auth, "\r\n") != 0) {
    auth = socket_read_line(sock);
    printf("CHECKING \"%s\"\n", auth);

    if (strcmp(auth, "\r\n") == 0) {
      break;
    }
    if (strcmp(auth, "Authorization: Basic YnJhZnRlcnk6d2lkZ2V0\r\n") == 0) {
      printf("AUTHORIZED\n");
      request->data = 0;
    }
  }
/*
  int ret_count = 0;

  printf("%s\n", auth);
  if (strcmp(auth, "\r\n") == 0) {
    ret_count = 5;
  }  
  printf("OUT: RET_COUNT = %d\n", ret_count);
  if (ret_count == 0) {
    new_char = socket_getc(sock);
    old_char = new_char;
  }



  while (ret_count < 5) {
    if (new_char == '\n' && old_char == '\r') {
      ret_count += 3;
    }
    else {
      ret_count -= 1;
      if (ret_count < 0) {
        ret_count = 0;
      }
    }

    old_char = new_char;

    if (ret_count == 5) {
      break;
    }

    if (new_char == -1) {
      break;
    }

    new_char = socket_getc(sock);
  }
*/  
  request->method = "GET";
  request->request_uri = strdup(doc_path);
  request->http_version = "HTTP/1.1";
}

int ends_with(char *file, char *ending) {
  int len = strlen(file);
  int end_len = strlen(ending);

  if ((len >= end_len) && (strcmp(file + len - end_len, ending) == 0)) {
    return 0;
  }

  return 1;
} /* ends_with() */


















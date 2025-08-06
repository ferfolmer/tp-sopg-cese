#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 5000
#define BUFFER_SIZE 1024
#define RESPONSE_OK "OK\n"
#define RESPONSE_NOTFOUND "NOTFOUND\n"
#define RESPONSE_ERROR "ERROR\n"

void send_error_and_exit(int client_fd, const char *msg) {
  perror(msg);
  send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
  exit(EXIT_FAILURE);
}

void handle_set(int client_fd, char *key, char *value) {
  if (!key || !value) {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    exit(EXIT_FAILURE);
  }
  FILE *fp = fopen(key, "w");
  if (!fp) {
    send_error_and_exit(client_fd, "fopen SET");
  }
  if (fprintf(fp, "%s", value) < 0) {
    fclose(fp);
    send_error_and_exit(client_fd, "fprintf SET");
  }
  fclose(fp);
  send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);
}

void handle_get(int client_fd, char *key) {
  if (!key) {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    exit(EXIT_FAILURE);
  }
  FILE *fp = fopen(key, "r");
  if (!fp) {
    if (errno == ENOENT) {
      send(client_fd, RESPONSE_NOTFOUND, strlen(RESPONSE_NOTFOUND), 0);
    } else {
      send_error_and_exit(client_fd, "fopen GET");
    }
    return;
  }
  char value[BUFFER_SIZE + 1];
  if (fgets(value, sizeof(value), fp) == NULL) {
    fclose(fp);
    send_error_and_exit(client_fd, "fgets GET");
  }
  fclose(fp);
  char *end = value + strlen(value) - 1;
  if (*end == '\n')
    *end = '\0';
  send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);
  send(client_fd, value, strlen(value), 0);
  send(client_fd, "\n", 1, 0);
}

void handle_del(int client_fd, char *key) {
  if (!key) {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    exit(EXIT_FAILURE);
  }
  if (remove(key) != 0 && errno != ENOENT) {
    send_error_and_exit(client_fd, "remove DEL");
  }
  send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);
}

// Atiende un único comando por conexión y luego retorna.
void serve_client(int client_fd) {
  char buffer[BUFFER_SIZE + 1];
  ssize_t n = recv(client_fd, buffer, BUFFER_SIZE, 0);
  if (n == -1) {
    perror("recv");
    return;
  }
  if (n == 0) {
    // Cliente cerró la conexión sin enviar datos
    return;
  }
  buffer[n] = '\0';

  char *newline = strchr(buffer, '\n');
  if (newline)
    *newline = '\0';

  char *command = strtok(buffer, " ");
  if (!command) {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    return;
  }

  if (strcmp(command, "SET") == 0) {
    char *key = strtok(NULL, " ");
    char *value = strtok(NULL, "");
    handle_set(client_fd, key, value);
  } else if (strcmp(command, "GET") == 0) {
    char *key = strtok(NULL, " ");
    handle_get(client_fd, key);
  } else if (strcmp(command, "DEL") == 0) {
    char *key = strtok(NULL, " ");
    handle_del(client_fd, key);
  } else {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    exit(EXIT_FAILURE);
  }
}

int main(void) {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int reuse = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(SERVER_PORT);

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    close(listen_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(listen_fd, 1) == -1) {
    perror("listen");
    close(listen_fd);
    exit(EXIT_FAILURE);
  }

  printf("Listening on port %d...\n", SERVER_PORT);

  while (1) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }
    serve_client(client_fd);
    close(client_fd);
  }

  close(listen_fd);
  return EXIT_SUCCESS;
}

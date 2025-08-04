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

// Atiende un único comando por conexión y luego retorna.
void serve_client(int client_fd) {
  char buffer[BUFFER_SIZE + 1];
  ssize_t n = recv(client_fd, buffer, BUFFER_SIZE, 0);
  if (n == -1) {
    // Error o cierre inmediato del cliente.
    perror("recv");
    exit(EXIT_FAILURE);
  }
  buffer[n] = '\0';

  char *newline = strchr(buffer, '\n');
  if (newline)
    *newline = '\0';

  char *command = strtok(buffer, " ");
  if (!command) {
    send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
    exit(EXIT_FAILURE);
  }

  if (strcmp(command, "SET") == 0) {
    char *key = strtok(NULL, " ");
    char *value = strtok(NULL, "");
    if (!key || !value) {
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      return;
    }
    FILE *fp = fopen(key, "w");
    if (!fp) {
      perror("fopen SET");
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      exit(EXIT_FAILURE);
    }
    if (fprintf(fp, "%s", value) < 0) {
      perror("fprintf SET");
      fclose(fp);
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      return;
    }
    fclose(fp);
    send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);

  } else if (strcmp(command, "GET") == 0) {
    char *key = strtok(NULL, " ");
    if (!key) {
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      return;
    }
    FILE *fp = fopen(key, "r");
    if (!fp) {
      if (errno == ENOENT) {
        send(client_fd, RESPONSE_NOTFOUND, strlen(RESPONSE_NOTFOUND), 0);
      } else {
        perror("fopen GET");
        send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
        exit(EXIT_FAILURE);
      }
      return;
    }
    char value[BUFFER_SIZE + 1];
    if (fgets(value, sizeof(value), fp) == NULL) {
      perror("fgets GET");
      fclose(fp);
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      exit(EXIT_FAILURE);
    }
    fclose(fp);
    char *end = value + strlen(value) - 1;
    if (*end == '\n')
      *end = '\0';

    send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);
    send(client_fd, value, strlen(value), 0);
    send(client_fd, "\n", 1, 0);

  } else if (strcmp(command, "DEL") == 0) {
    char *key = strtok(NULL, " ");
    if (!key) {
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      return;
    }
    if (remove(key) != 0 && errno != ENOENT) {
      perror("remove DEL");
      send(client_fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0);
      exit(EXIT_FAILURE);
    }
    send(client_fd, RESPONSE_OK, strlen(RESPONSE_OK), 0);

  } else {
    // Comando desconocido
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

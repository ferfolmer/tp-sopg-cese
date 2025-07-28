/**
 * @file trabajo-practico.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-21
 *
 * @copyright Copyright (c) 2025
 *
 * Trabajo práctico
Objetivo

Escribir un servidor TCP que permite almacenar información ASCII en forma de
clave-valor.

El servidor debe:

    Esperar a que un cliente se conecte mediante el protocolo TCP, puerto 5000.

    Esperar a que el cliente envíe un comando a ejecutar. El comando se
especifica como una secuencia de caracteres ASCII hasta el \n.

    Realizar la operación correspondiente.

    Cortar la conexión con el cliente y volver al paso 1.

Los comandos que acepta el servidor son:

    SET <clave> <valor>\n:
        Se crea en el servidor un archivo llamado <clave> con el contenido
indicado en <valor> (sin incluir el \n). Se responde al cliente OK\n. GET
<clave>: Si existe el archivo correspondiente, se responde al cliente con:
OK\n<valor>\n (es decir, una línea de texto que dice OK y otra que contiene el
contenido del archivo). Si no existe, se responde con NOTFOUND\n DEL <clave>: Si
existe la el archivo correspondiente, se elimina. Tanto si existe como no, se
responde OK\n.

// ...// ...// ...// ...existing code...
if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
}
// ...existing code...
if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
0) { perror("Bind failed"); close(server_socket); exit(EXIT_FAILURE);
}
// ...existing code...
if (listen(server_socket, 5) < 0) {
    perror("Listen failed");
    close(server_socket);
    exit(EXIT_FAILURE);
}
// ...existing code...
if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
&addr_len)) < 0) { perror("Accept failed"); exit(EXIT_FAILURE);
}
// ...existing code...Cliente

Dado que el protocolo de comunicación es ASCII, no es necesario programar un
cliente sino que se pueden utilizar herramientas como nc (netcat) o telnet.

En ubuntu se pueden instalar con: apt install netcat o apt install telnet.
Ejemplo

    En la consola #1 (server): ./server

    En la consola #2 (client): nc localhost 5000. Si la conexión es exitosa, el
proceso se queda esperando a recibir entrada de stdin.

$ nc localhost 5000
SET manzana apple
OK
$ nc localhost 5000
SET perro dog
OK
$ nc localhost 5000
SET hola hello
OK
$ nc localhost 5000
GET perro
OK
dog
$ nc localhost 5000
GET casa
NOTFOUND
$ nc localhost 5000
DEL perro
OK
$ nc localhost 5000
GET perro
NOTFOUND
$

 *
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define ERROR_STR "ERROR\n"
#define NOTFOUND_STR "NOTFOUND\n"
#define OK_STR "OK\n"
#define UNKNOWN_COMMAND_STR "UNKNOWN COMMAND\n"

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(client_socket, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0'; // Null-terminate the string
    char *command = strtok(buffer, " \n");

    if (strcmp(command, "SET") == 0) {
      char *key = strtok(NULL, " ");
      char *value = strtok(NULL, "\n");
      if (key && value) {
        FILE *file = fopen(key, "w");
        if (file) {
          fprintf(file, "%s", value);
          fclose(file);
          write(client_socket, OK_STR, strlen(OK_STR));
        } else {
          perror("Error creating file");
          write(client_socket, ERROR_STR, strlen(ERROR_STR));
          exit(EXIT_FAILURE); // Termina el proceso ante error crítico
        }
      } else {
        write(client_socket, ERROR_STR, strlen(ERROR_STR));
        exit(EXIT_FAILURE); // Termina el proceso ante error crítico
      }
    } else if (strcmp(command, "GET") == 0) {
      char *key = strtok(NULL, "\n");
      if (key) {
        FILE *file = fopen(key, "r");
        if (file) {
          char value[BUFFER_SIZE];
          if (fgets(value, sizeof(value), file) == NULL) {
            perror("Error reading file");
            fclose(file);
            write(client_socket, ERROR_STR, strlen(ERROR_STR));
            exit(EXIT_FAILURE);
          }
          fclose(file);
          write(client_socket, OK_STR, strlen(OK_STR));

          char response[BUFFER_SIZE + 2];
          snprintf(response, sizeof(response), "%s\n", value);
          write(client_socket, response, strlen(response));
        } else {
          perror("Error opening file for GET");
          write(client_socket, NOTFOUND_STR, strlen(NOTFOUND_STR));
          exit(EXIT_FAILURE);
        }
      } else {
        write(client_socket, ERROR_STR, strlen(ERROR_STR));
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(command, "DEL") == 0) {
      char *key = strtok(NULL, "\n");
      if (key) {
        if (remove(key) == 0) {
          write(client_socket, OK_STR, strlen(OK_STR));
        } else {
          perror("Error deleting file");
          write(client_socket, ERROR_STR, strlen(ERROR_STR));
          exit(EXIT_FAILURE);
        }
      } else {
        write(client_socket, ERROR_STR, strlen(ERROR_STR));
        exit(EXIT_FAILURE);
      }
    } else {
      write(client_socket, UNKNOWN_COMMAND_STR, strlen(UNKNOWN_COMMAND_STR));
      exit(EXIT_FAILURE);
    }
  }

  close(client_socket);
}

int main() {
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

  // Create socket
  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Set up the server address structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // Bind the socket
  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Bind failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Listen for incoming connections
  if (listen(server_socket, 1) == -1) {
    perror("Listen failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d\n", PORT);

  while (1) {
    // Accept a new client connection
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
      perror("Accept failed");
      exit(EXIT_FAILURE);
    }

    printf("Client connected\n");

    // Handle the client in a separate function
    handle_client(client_socket);
  }

  close(server_socket);
  return 0;
}

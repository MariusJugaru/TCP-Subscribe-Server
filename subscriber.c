/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include "common.h"
#include "helpers.h"

void send_packet(int sockfd, char *message, int type) {
  struct tcp_packet sent_packet;

  sent_packet.tip_date = type;
  strcpy(sent_packet.message, message);

  send(sockfd, &sent_packet, sizeof(sent_packet), 0);
}

void run_client(int sockfd, char *id) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct tcp_packet sent_packet;
  struct tcp_packet recv_packet;

  fd_set read_fds;
  int max_fd;

  if (sockfd > fileno(stdin)) {
    max_fd = sockfd;
  } else {
    max_fd = fileno(stdin);
  }

  while (1) {
    memset(buf, 0, MSG_MAXSIZE + 1);
    FD_ZERO(&read_fds);
    FD_SET(fileno(stdin), &read_fds);
    FD_SET(sockfd, &read_fds);

    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    // Verificăm dacă avem input de la tastatură
    if (FD_ISSET(fileno(stdin), &read_fds)) {
      if (!fgets(buf, sizeof(buf), stdin) || isspace(buf[0])) {
        send_packet(sockfd, id, 3);
        break;
      }
      buf[strlen(buf) - 1] = '\0';
      // printf("%s len %ld\n", buf, strlen(buf));

      if (strcmp(buf, "exit") == 0) {
        break;
      }
      
      char command[20];
      memset(command, 0, 20);

      if (strchr(buf, ' ') == NULL) {
        printf("\n Usage: subscribe/unsubscribe <TOPIC>\n");
        send_packet(sockfd, id, 3);
        break;
      }
      strncpy(command, buf, strchr(buf, ' ') - buf);
      if (strcmp(command, "subscribe") == 0) {
        send_packet(sockfd, buf + strlen("subscribe") + 1, 1);
        printf("Subscribed to topic %s\n", buf + strlen("subscribe") + 1);
        continue;
      }

      if (strcmp(command, "unsubscribe") == 0) {
        send_packet(sockfd, buf + strlen("unsubscribe") + 1, 2);
        printf("Unsubscribed from topic %s\n", buf + strlen("unsubscribe") + 1);
        continue;;
      }
      strcpy(sent_packet.message, buf);

      send(sockfd, &sent_packet, sizeof(sent_packet), 0);
    }

    // Verificăm dacă avem mesaj de la server
    if (FD_ISSET(sockfd, &read_fds)) {
      int rc = recv(sockfd, &recv_packet, sizeof(recv_packet), 0);
      if (rc <= 0) {
        if (rc == 0) {
          // printf("Server disconnected.\n");
        } else {
          perror("recv");
        }
        break;
      }
      DIE(recv_packet.tip_date != 3, "wrong message");
      
      printf("%s\n", recv_packet.message);
      }
    }
}

int open_and_connect_sock(int port, char* ip) {
  int rc;

  // Obtinem un socket TCP pentru conectarea la server
  const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  return sockfd;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
    return 1;
  }

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  char id[64];
  rc = sscanf(argv[1], "%s", id);

  int sockfd = open_and_connect_sock(port, argv[2]);
  struct tcp_packet sent_packet;
  strcpy(sent_packet.message, argv[1]);
  sent_packet.tip_date = 0;

  send(sockfd, &sent_packet, sizeof(sent_packet), 0);
  
  run_client(sockfd, id);

  // Inchidem conexiunea si socketul creat
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);

  system("exit");
  return 0;
}

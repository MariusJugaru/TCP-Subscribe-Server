/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <netinet/tcp.h>

#include "common.h"
#include "helpers.h"
#include <sys/select.h>
#include <math.h>

void add_pollin(struct pollfd poll_fds[], int sockfd, int *num_sockets, int max_connections) {
  poll_fds[*num_sockets].fd = sockfd;
  poll_fds[*num_sockets].events = POLLIN;
  (*num_sockets)++; 
}

void run_chat_multi_server(int sockfd_tcp, int sockfd_udp) {
  int max_connections = 32;

  struct pollfd *poll_fds;
  struct tcp_packet received_packet_tcp;
  struct tcp_packet send_packet_tcp;
  struct udp_packet received_packet_udp;
  char id[11];
  int rc;

  poll_fds = (struct pollfd *)malloc(max_connections * sizeof(struct pollfd));
  DIE(poll_fds == NULL, "poll malloc");

  struct cel *List = NULL;
  
  int run_server = 1;
  int num_sockets = 0;
  
  // Setam socket-ul listenfd pentru ascultare
  rc = listen(sockfd_tcp, max_connections);
  DIE(rc < 0, "listen");

  // Adaugam noii file descriptori in
  // multimea poll_fds
  add_pollin(poll_fds, fileno(stdin), &num_sockets, max_connections);
  add_pollin(poll_fds, sockfd_tcp, &num_sockets, max_connections);
  add_pollin(poll_fds, sockfd_udp, &num_sockets, max_connections);
  FILE *fp = fopen("testttt.txt", "w+");

  while (run_server) {
    // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_sockets; i++) {
      fprintf(fp, "SOCKET: %d\n", i);
      
      if (poll_fds[i].revents & POLLIN) {
        fprintf(fp, "POLLIN: %d\n", i);
        if (poll_fds[i].fd == sockfd_tcp) {
          memset(&received_packet_tcp, 0, sizeof(struct tcp_packet));

          fprintf(fp, "TCP: %d\n", i);
          // Am primit o cerere de conexiune pe socketul de listen, pe care
          // o acceptam
          struct sockaddr_in cli_addr;
          struct tcp_packet recv_packet;
          
          socklen_t cli_len = sizeof(cli_addr);
          const int newsockfd =
              accept(sockfd_tcp, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          rc = recv_all(newsockfd, &recv_packet, sizeof(recv_packet));
          DIE(recv_packet.tip_date != 0, "error");

          strcpy(id, recv_packet.message);

          if (List == NULL) {
            List = (struct cel*)malloc(sizeof(struct cel));

            strcpy(List->id, id);
            List->fd = newsockfd;

            rc = 0;
          } else {
            rc = add_id(id, List, newsockfd);
          }

          if (rc == 0) {
            printf("New client %s connected from %s:%d.\n",
                 id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            // Adaugam noul socket intors de accept() la multimea descriptorilor
            // de citire
            if (num_sockets == max_connections) {
              // Max connections reached, we double the fd array
              max_connections *= 2;
              poll_fds = realloc(poll_fds, max_connections * sizeof(struct pollfd));
              DIE(poll_fds == NULL, "poll realloc");

              rc = listen(sockfd_tcp, max_connections);
              DIE(rc < 0, "listen");
            }

            add_pollin(poll_fds, newsockfd, &num_sockets, max_connections);
          } else {
            printf("Client %s already connected.\n", id);
            close(newsockfd);
          }
          break;
        } else if (poll_fds[i].fd == sockfd_udp) {
          memset(&received_packet_udp, 0, sizeof(received_packet_udp));
          memset(&send_packet_tcp, 0, sizeof(struct tcp_packet));

          /* Receive a datagram and send an ACK */
          /* The info of the who sent the datagram (PORT and IP) */
          struct sockaddr_in client_addr;
          socklen_t clen = sizeof(client_addr);
          rc = recvfrom(poll_fds[i].fd, &received_packet_udp, sizeof(struct udp_packet), 0,
                            (struct sockaddr *)&client_addr, &clen);

          // printf("New client %s connected from %s:%d.\n",
          //        id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

          /* We know it's a string so we print it*/
          // printf("[Server] Received: %s %d\n", received_packet_udp.topic,
          //                           received_packet_udp.tip_date);

          int ack = 0;
          // Sending ACK. We model ACK as datagrams with only an int of value 0.
          rc = sendto(poll_fds[i].fd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr,
                      clen);
          DIE(rc < 0, "send");

          unsigned char sign_byte;
          uint32_t val_32;
          uint16_t val_16;
          uint8_t val_8;

          struct queue *q = NULL;

          for (struct cel *p_client = List; p_client != NULL; p_client = p_client->next) {
            for (struct subscription *p_sub = p_client->subscriptions; p_sub != NULL; p_sub = p_sub->next) {
              if (strcmp(p_sub->topic, received_packet_udp.topic) == 0) {
                add_queue(&q, p_client->fd);
                break;
              }
            }
          }

          int fd_aux;

          send_packet_tcp.tip_date = 3;
          switch (received_packet_udp.tip_date)
          {
          case 0:
            sign_byte = received_packet_udp.continut[0];
            memcpy(&val_32, received_packet_udp.continut + 1, sizeof(uint32_t));
            val_32 = ntohl(val_32);

            if (sign_byte) {
              val_32 *= -1;
            }

            sprintf(send_packet_tcp.message, "%s:%d - %s - INT - %d",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                    received_packet_udp.topic, val_32);
            // printf("Val: %d\n", val_32);
            break;
          case 1:
            float val_sh_real;

            memcpy(&val_16, received_packet_udp.continut, sizeof(uint16_t));
            val_sh_real = (float)(ntohs(val_16)) / 100;

            sprintf(send_packet_tcp.message, "%s:%d - %s - SHORT_REAL - %.2f",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                    received_packet_udp.topic, val_sh_real);
            // printf("Val: %.2f\n", val_sh_real);
            break;
          case 2:
            float val_float;
            sign_byte = received_packet_udp.continut[0];

            memcpy(&val_32, received_packet_udp.continut + 1, sizeof(uint32_t));
            val_32 = ntohl(val_32);

            memcpy(&val_8, received_packet_udp.continut + 1 +
                   sizeof(uint32_t), sizeof(uint8_t));

            float fact = pow(10, val_8);
            val_float = ((float)val_32 / fact);
            if (sign_byte) {
              val_float *= -1;
            }

            char rounded_str[20];
            sprintf(rounded_str, "%.*f", val_8, val_float);

            sprintf(send_packet_tcp.message, "%s:%d - %s - FLOAT - %s",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                    received_packet_udp.topic, rounded_str);
            // printf("Val: %s\n", rounded_str);
            break;
          case 3:
            sprintf(send_packet_tcp.message, "%s:%d - %s - STRING - %s",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                    received_packet_udp.topic, received_packet_udp.continut);
            // printf("%s\n", received_packet_udp.continut);
            break;
          default:
            break;
          }

          while (q != NULL) {
            fd_aux = pop_queue(&q);

            send(fd_aux, &send_packet_tcp, sizeof(struct tcp_packet), 0);
          }
        } else if (poll_fds[i].fd == fileno(stdin)) {
          char buf[256];

          fgets(buf, sizeof(buf), stdin);

          if (strcmp("exit\n", buf) == 0) {
            run_server = 0;
            break;
          }
          break;
        } else {
          fprintf(fp, "TCP_SOCK: %d\n", i);
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          int rc = recv_all(poll_fds[i].fd, &received_packet_tcp,
                            sizeof(received_packet_tcp));
          DIE(rc < 0, "recv");

          struct cel *aux = find_by_fd(List, poll_fds[i].fd);

          // Client has disconnected
          if (rc == 0) {
            DIE(aux == NULL, "fd does not exist");
            aux->fd = -1;

            printf("Client %s disconnected.\n", aux->id);

            shutdown(poll_fds[i].fd, SHUT_RDWR);
            close(poll_fds[i].fd);

            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }
            num_sockets--;

            break;
          }
          
          char topic[51];
          // printf("%d\n", received_packet.len);
          strcpy(topic, received_packet_tcp.message);

          struct subscription *subscription;
          struct subscription *pre;

          switch (received_packet_tcp.tip_date) {
          case 1:
            for (subscription = aux->subscriptions, pre = NULL;
                subscription != NULL; pre = subscription, subscription = subscription->next) {
              if (strcmp(subscription->topic, topic) == 0) {
                //Already subscribed
                break;
              }
            }

            if (subscription == NULL) {
              if (pre == NULL) {
                aux->subscriptions = (struct subscription*)malloc(sizeof(struct subscription));
                strcpy(aux->subscriptions->topic, topic);
                aux->subscriptions->next = NULL;
              } else {
                pre->next = (struct subscription*)malloc(sizeof(struct subscription));
                strcpy(pre->next->topic, topic);
                pre->next->next = NULL;
              }
            }
            break;
          case 2:
            for (subscription = aux->subscriptions, pre = NULL;
                subscription != NULL; pre = subscription, subscription = subscription->next) {
              if (strcmp(subscription->topic, topic) == 0) {
                if (pre == NULL) {
                  aux->subscriptions = subscription->next;
                  free(subscription);
                } else {
                  pre->next = subscription->next;
                  free(subscription);
                }
                break;
              }
            }
            break;
          default:
            break;
          }
        }
      }
    }
  }

  for (int i = 3; i < num_sockets; i++) {
    close(poll_fds[i].fd);
  }
  fclose(fp);
}

int open_and_bind_socket(uint16_t port, int connection_type) {
  // Obtinem un socket pentru receptionarea conexiunilor
  const int sockfd = socket(AF_INET, connection_type, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  const int enable = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  // Asociem adresa serverului cu socketul creat folosind bind
  int rc = bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind");

  return sockfd;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("\n Usage: %s <port>\n", argv[0]);
    return 1;
  }
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  const int sockfd_tcp = open_and_bind_socket(port, SOCK_STREAM);
  const int sockfd_udp = open_and_bind_socket(port, SOCK_DGRAM);

  run_chat_multi_server(sockfd_tcp, sockfd_udp);

  close(sockfd_tcp);
  close(sockfd_udp);

  return 0;
}

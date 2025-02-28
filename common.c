#include "common.h"
#include "helpers.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_received_total = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  
  while(bytes_remaining) {
    bytes_received = recv(sockfd, buff + bytes_received_total, bytes_remaining, 0);
    DIE(bytes_received == -1, "recv");
    if (bytes_received == 0) {
      break;
    }

    bytes_received_total += bytes_received;
    bytes_remaining -= bytes_received;
  }

  return bytes_received_total;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_sent_total = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while(bytes_remaining) {
    bytes_sent = send(sockfd, buff + bytes_sent_total, bytes_remaining, 0);
    DIE(bytes_sent == -1, "send");

    bytes_sent_total += bytes_sent;
    bytes_remaining -= bytes_sent;
  }

  return bytes_sent_total;
}

int add_id(char id[], struct cel *List, int fd) {
  struct cel *p, *pre;

  for (pre = NULL, p = List; p != NULL; pre = p, p = p->next) {
    if (strcmp(p->id, id) == 0) {
      if (p->fd == -1) {
        p->fd = fd;
        return 0;
      } else {
        return -1;
      }
    }
  }

  struct cel* new_id = (struct cel*)malloc(sizeof(struct cel));
  strcpy(new_id->id, id);
  new_id->fd = fd;
  pre->next = new_id;

  return 0;
}

struct cel* find_by_fd(struct cel *List, int fd) {
  struct cel *p;

  for (p = List; p != NULL; p = p->next) {
    if (p->fd == fd) {
      return p;
    }
  }

  return NULL;
}

void add_queue(struct queue **q, int val) {
  struct queue *node = (struct queue *)malloc(sizeof(struct queue));
  DIE(node == NULL, "queue add");
  node->val = val;

  if (*q == NULL) {
    *q = node;
  } else {
    struct queue *p = *q;
    while (p->next != NULL) {
      p = p->next;
    }

    p->next = node;
  }
}

int pop_queue(struct queue **q) {
    if (*q == NULL) {
        fprintf(stderr, "Eroare: coada este goală\n");
        exit(EXIT_FAILURE);
    }
    int val = (*q)->val;
    struct queue *p = *q;
    *q = (*q)->next;
    free(p);
    return val;
}
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1500

/*
tip_date tcp
0 - ID subscriber
1 - subscribe
2 - unsubscribe
3 - messsage from server
*/
struct tcp_packet {
  unsigned char tip_date;
  char message[MSG_MAXSIZE + 100];
};

struct udp_packet {
  char topic[50];
  unsigned char tip_date;
  char continut[MSG_MAXSIZE + 1];
};

struct cel {
  char id[11];
  int fd;
  struct cel *next;
  struct subscription* subscriptions;
};

struct subscription {
  char topic[50];
  struct subscription *next;
};

struct queue {
  int val;
  struct queue *next;
};

int add_id(char id[], struct cel *List, int fd);
struct cel* find_by_fd(struct cel *List, int fd);
void add_queue(struct queue **q, int val);
int pop_queue(struct queue **q);

#endif

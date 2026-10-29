/* CSD 304 Computer Networks, Fall 2016
   Lab 4, Sender
   Team:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MC_PORT 5432
#define BUF_SIZE 64000

static char buf[BUF_SIZE];

/* Multicast one file: first the number of packets, then the packets themselves. */
static void send_file(int s, struct sockaddr_in *sin, const char *name, useconds_t delay)
{
  socklen_t sin_len = sizeof(*sin);
  FILE *fp = fopen(name, "rb");

  if (fp == NULL)
  {
    const char *msg = "File Not Found\n";
    sendto(s, msg, strlen(msg)+1, 0, (struct sockaddr*)sin, sin_len);
    return;
  }

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  int tot_frame = (int)((fsize + BUF_SIZE - 1) / BUF_SIZE);

  printf("last packets are :%ld\n\n", fsize % BUF_SIZE);
  printf("Total number of packets are :%d\n\n", tot_frame);

  if (sendto(s, &tot_frame, sizeof(tot_frame), 0, (struct sockaddr*)sin, sin_len) < 0)
    printf("Error in sending frame no:\n");

  for (int i = 1; i <= tot_frame; i++)
  {
    size_t len = fread(buf, 1, BUF_SIZE, fp);
    if (len == 0)
      break;
    if (sendto(s, buf, len, 0, (struct sockaddr*)sin, sin_len) < 0)
      perror("sender: sendto");
    printf("sent frame %d\n", i);
    usleep(delay);
  }

  fclose(fp);
}

int main(int argc, char * argv[]){

  int s,s_tcp; /* socket descriptor */
  struct sockaddr_in sin,sin_t; /* socket struct */
  socklen_t sin_len = sizeof(sin_t);
  /* Multicast specific */
  char *mcast_addr; /* multicast address */
  int tcp_ac;

  if (argc==2) {
    mcast_addr = argv[1];
  }
  else {
    fprintf(stderr, "usage: sender multicast_address\n");
    exit(1);
  }

//////////////////////////////////////////////////////////////////////////////////////////////////

  /* build address data structure for TCP*/
  bzero((char *)&sin_t, sizeof(sin_t));
  sin_t.sin_family = AF_INET;
  sin_t.sin_addr.s_addr = INADDR_ANY;
  sin_t.sin_port = htons(MC_PORT);

  /* Create a TCP socket */
  if ((s_tcp = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("server TCP: socket");
    exit(1);
  }
  printf("Socket created\n\n");

  int opt = 1;
  setsockopt(s_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // binding server addr structure to s_tcp
  if ((bind(s_tcp, (struct sockaddr *) &sin_t, sizeof(sin_t))) < 0) {
    perror("receiver: bind()");
    close(s_tcp);
    exit(1);
  }
  printf("Binded\n\n");

  if (listen(s_tcp, 5) < 0) {
    perror("Error in listening");
    close(s_tcp);
    exit(1);
  }
  printf("Listened\n\n");

  if ((tcp_ac = accept(s_tcp, (struct sockaddr*)&sin_t, &sin_len)) < 0) {
    perror("Error in accepting");
    close(s_tcp);
    exit(1);
  }
  printf("Accepted\n\n");

  int num = 0;
  /* read exactly sizeof(num) bytes: the old sizeof(num)+1 overflowed 'num' */
  if (recv(tcp_ac, &num, sizeof(num), MSG_WAITALL) != (ssize_t)sizeof(num)) {
    fprintf(stderr, "Error receiving station number\n");
    close(tcp_ac);
    close(s_tcp);
    exit(1);
  }
  printf("Station number is: %d\n\n",num);

//////////////////////////////////////////////////////////////////////////////////////////////////

  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server UDP: socket");
    exit(1);
  }

  // build address data structure
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  if (inet_pton(AF_INET, mcast_addr, &sin.sin_addr) != 1) {
    fprintf(stderr, "sender: invalid multicast address '%s'\n", mcast_addr);
    exit(1);
  }
  sin.sin_port = htons(MC_PORT);

  send_file(s, &sin, "vid1.mp4", 200000);

  printf("\tCurrent Stream: va1.mp4\n\n");
  printf("\tNext Stream: xyz.mp4\n\n");
  send_file(s, &sin, "va1.mp4", 200000);

  send_file(s, &sin, "cn.mp4", 10000);

  close(s);
  close(tcp_ac);
  close(s_tcp);
  return 0;
}

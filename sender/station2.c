//sender_udp2 with gtk  (station 2)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MC_PORT 5433
#define BUF_SIZE 64000
#define NUM_VIDEOS 4
#define MC_TTL 1

int main(int argc, char * argv[])
{
	  int s; // socket descriptor
	  struct sockaddr_in sin; // socket struct
	  static char buf[BUF_SIZE];
	  socklen_t sin_len = sizeof(sin);

	  // Multicast specific
	  char *mcast_addr; // multicast address

	  //Array of video names
	  const char *video[NUM_VIDEOS] = { "vid5.mp4", "vid6.mp4", "vid7.mp4", "vid8.mp4" };

	  if (argc==2)
	  {
			mcast_addr = argv[1];
	  }
	  else
	  {
			fprintf(stderr, "usage: station2 multicast_address\n");
			exit(1);
	  }

	  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	  {
			perror("server UDP: socket");
			exit(1);
	  }

	  // build address data structure
	  memset((char *)&sin, 0, sizeof(sin));
	  sin.sin_family = AF_INET;
	  if (inet_pton(AF_INET, mcast_addr, &sin.sin_addr) != 1)
	  {
			fprintf(stderr, "station2: invalid multicast address '%s'\n", mcast_addr);
			exit(1);
	  }
	  sin.sin_port = htons(MC_PORT);

	  // TTL for outgoing multicast datagrams (1 = stay on the local network)
	  unsigned char ttl = MC_TTL;
	  if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
			perror("station2: setsockopt(IP_MULTICAST_TTL)");

	  printf("Connected in second station\n\n ");

	  while(1)
	  {
		  int sent_any = 0;

		  for(int v=0; v<NUM_VIDEOS; v++)	//Sending videos one by one
		  {
			  FILE *fp = fopen(video[v],"rb");

			  if(fp==NULL)		//Check if file exist
			  {
				  printf("\nFile not found: %s\n", video[v]);
				  continue;
			  }

			  sent_any = 1;

			  fseek(fp, 0, SEEK_END);
			  long fsize = ftell(fp);		//Calculate file size
			  fseek(fp, 0, SEEK_SET);

			  long p = fsize % BUF_SIZE;
			  long tot_frame = (fsize + BUF_SIZE - 1) / BUF_SIZE;

			  printf("last packets are :%ld\n\n", p);
			  printf("Total number of packets are :%ld\n\n", tot_frame);

			  for(long i=1; i<=tot_frame; i++)	//Sending data frame by frame
			  {
				  size_t len = fread(buf,1,BUF_SIZE,fp);
				  if (len == 0)
					  break;
				  if (sendto(s,buf,len,0,(struct sockaddr*)&sin,sin_len) < 0)
					  perror("station2: sendto");
				  printf("sent frame %ld\n",i);
				  usleep(300000);
			  }
			  fclose(fp); //close file pointer
		  }

		  if (!sent_any)	//No file could be opened: don't spin the CPU
		  {
			  fprintf(stderr, "station2: none of the video files could be opened\n");
			  sleep(5);
		  }
	  }
	  close(s);
	  return 0;
}

//Receiver with gtk
//usage: receiver multicast_address [network_interface]
//  network_interface is optional (e.g. wlan0, eth0). If omitted, the socket is
//  not bound to a device (this also removes the need for sudo). It can also be
//  given through the RADIO_IFACE environment variable.
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

#define MC_PORT 5433
#define BUF_SIZE 64000

//structure of song info
struct song_info
{
	  char song_name[ 50 ];
	  uint16_t remaining_time_in_sec;
	  char next_song_name[ 50 ];
};

/* shared between the GTK thread and the receiving thread */
static volatile int done = 0;			//1 = paused
static const char *g_mcast_addr = NULL;
static const char *g_if_name = NULL;

/* Function for Pause */
void func1(GtkWidget *widget,gpointer   data)
{
   g_print ("video is pause...\n");
   done=1;
}

/* Function for Resume */
void func2(GtkWidget *widget,gpointer   data)
{
   g_print ("video resumed...\n");
   done=0;
}

/* Function for Change Station */
void func3(GtkWidget *widget,gpointer   data)
{
   g_print ("Request to change the station\n");
   system("pkill ffplay");
   remove("live_data.mp4");
   exit(0);
}

/* Function for Terminate */
void func4(GtkWidget *widget,gpointer   data)
{
  g_print ("Terminated from current station...\nBYE BYE...\n");
  system("pkill ffplay");
  remove("live_data.mp4");
  exit(0);
}

void* threadFunction(void* args)
{
  printf("in thread\n");

  int s; /* socket descriptor */
  struct sockaddr_in sin; /* socket struct */
  struct ifreq ifr; /* interface struct */
  char buf[BUF_SIZE];
  int len;
  /* Multicast specific */
  struct ip_mreq mcast_req;  /* multicast join struct */
  struct sockaddr_in mcast_saddr; /* multicast sender*/
  socklen_t mcast_saddr_len;

  /* create socket */
  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
  {
		perror("receiver: socket");
		exit(1);
  }
  else
  printf("udp Socket created\n");

  /* allow several receivers on the same machine / quick restarts */
  int reuse = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  /* build address data structure */
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(MC_PORT);

  /*Use the interface specified (optional) */
  if (g_if_name != NULL && g_if_name[0] != '\0')
  {
      memset(&ifr, 0, sizeof(ifr));
      strncpy(ifr.ifr_name , g_if_name, IFNAMSIZ-1);

      if ((setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr))) < 0)
      {
          perror("receiver: setsockopt(SO_BINDTODEVICE) error (needs root, and the interface must exist)");
          close(s);
          exit(1);
      }
      else
          printf("bound to interface %s\n", g_if_name);
  }

  /* bind the socket */
  if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0)
  {
    perror("receiver: bind()");
    close(s);
    exit(1);
  }
  else
  printf("udp binded\n");
  /* Multicast specific code follows */

  /* build IGMP join message structure */
  mcast_req.imr_multiaddr.s_addr = inet_addr(g_mcast_addr);
  mcast_req.imr_interface.s_addr = htonl(INADDR_ANY);

  /* send multicast join message */
  if ((setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcast_req, sizeof(mcast_req))) < 0)
  {
    perror("mcast join receive: setsockopt()");
    exit(1);
  }

  /* receive multicast messages */
  printf("\nReady to listen!\n\n");
  int i=0;
  int player_started = 0;
  FILE *fp;
  fp=fopen("live_data.mp4","wb");
  if (fp == NULL)
  {
    perror("receiver: fopen(live_data.mp4)");
    exit(1);
  }

  /* reset sender struct */
  memset(&mcast_saddr, 0, sizeof(mcast_saddr));
  mcast_saddr_len = sizeof(mcast_saddr);

  while(1)
  {
    	if(done==0)
      {
             len = recvfrom(s, buf, sizeof(buf), 0,(struct sockaddr*)&mcast_saddr, &mcast_saddr_len);

             if(len<0)
             {
                 if (errno != EINTR)
                     perror("Error in receiving");
                 usleep(10000);
             }
             else
             {
                 printf("%d Receiving  %d\n",i,len);
                 fwrite(buf,1,len,fp);
                 fflush(fp);	//make the data visible to ffplay, which reads the file while we write it
                 if(i>=8 && !player_started)
            	  {
							player_started = 1;
							system("ffplay -i live_data.mp4 &");
					  }
                 i++;
             }
        }
        else
        {
             usleep(100000);	//paused: don't spin the CPU
        }
  }

  fclose(fp);
  close(s);
  return NULL;
}

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
    {
		fprintf(stderr, "usage: receiver multicast_address [network_interface]\n");
		exit(1);
    }
    g_mcast_addr = argv[1];
    g_if_name = (argc == 3) ? argv[2] : getenv("RADIO_IFACE");

    pthread_t id;
    pthread_create(&id,NULL,&threadFunction,NULL);

    gtk_init (&argc, &argv);
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid;
    GtkWidget *button;
    GdkColor color;

    gtk_window_set_title (GTK_WINDOW (window), "Control!");
    gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);		//Set window size
    gdk_color_parse ("light yellow", &color);		//Set color of GUI window
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    grid = gtk_grid_new ();

    gtk_container_add (GTK_CONTAINER (window), grid);
    gtk_widget_modify_bg ( GTK_WIDGET(window), GTK_STATE_NORMAL, &color);


    button = gtk_button_new_with_label ("Pause");

    g_signal_connect (button, "clicked", G_CALLBACK (func1), NULL);	//call function 1 for pause

    gtk_grid_attach (GTK_GRID (grid), button, 5, 5, 5, 5);

    button = gtk_button_new_with_label ("Resume");
    g_signal_connect (button, "clicked", G_CALLBACK (func2), NULL);	//call function 2 for resume

    gtk_grid_attach (GTK_GRID (grid), button, 5, 10, 5, 5);

    button = gtk_button_new_with_label ("Change station");
	 g_signal_connect (button, "clicked", G_CALLBACK (func3), NULL);	//call function 3 for change station

    gtk_grid_attach (GTK_GRID (grid), button, 5, 15, 5, 5);

    button = gtk_button_new_with_label ("Terminate");
    g_signal_connect (button, "clicked", G_CALLBACK (func4), NULL);	//call function 4 for teminate

    gtk_grid_attach (GTK_GRID (grid), button, 5, 20, 5, 5);


    gtk_widget_show_all (window);
    gtk_main ();

    /* window closed: stop the player and clean up as "Terminate" does */
    system("pkill ffplay");
    remove("live_data.mp4");
    return 0;
}

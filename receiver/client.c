//Client_with gtk

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MC_PORT 5432

struct site_info
{
	uint8_t site_name_size;
	char site_name[ 20 ];
	uint8_t site_desc_size;
	char site_desc[ 100 ];
	uint8_t station_count;
};

struct station_info
{
	uint8_t station_number;
	char station_name[50];
	char multicast_address[32];
	uint16_t data_port;
	uint16_t info_port;
	uint32_t bit_rate;
};

/* what a station button needs: the status label and the station to tune to */
struct button_data
{
	GtkLabel *label;
	char address[32];	/* same size as station_info.multicast_address */
	int number;
};

static struct button_data bd1, bd2;

/* receive exactly len bytes from a TCP socket (recv() may return less) */
static int recv_all(int fd, void *data, size_t len)
{
	return recv(fd, data, len, MSG_WAITALL) == (ssize_t)len ? 0 : -1;
}

static void tune_to(struct button_data *bd)
{
	char display[64];
	char cmd[200];
	struct in_addr tmp;

	/* the address came over the network and ends up in a shell command: validate it */
	if (inet_pton(AF_INET, bd->address, &tmp) != 1)
	{
		gtk_label_set_text(bd->label, "Invalid station address");
		return;
	}

	snprintf(display, sizeof(display), "Connected to Station %d!", bd->number);
	gtk_label_set_text(bd->label, display);
	while(gtk_events_pending())
		gtk_main_iteration();

	system("gcc -Wno-deprecated-declarations `pkg-config --cflags gtk+-3.0` -o receiver receiver.c -pthread `pkg-config --libs gtk+-3.0` ");
	snprintf(cmd, sizeof(cmd), "./receiver %s", bd->address);
	system(cmd);
}

/* function for button 1 "station 1" and button 2 "station 2".
   GTK calls "clicked" handlers as (widget, user_data): user_data is our button_data */
static void on_station(GtkWidget *widget, gpointer data)
{
	tune_to((struct button_data *)data);
}

static void on_exit_clicked(GtkWidget *widget, gpointer data)
{
	exit(0);
}

static void print_station(const struct station_info *st)
{
	printf("\n\n-------------------------------------------------------\n");
	printf("info port        : %d\n", st->info_port);
	printf("Station Number   : %d\n", st->station_number);
	printf("Station name     : %s\n", st->station_name);
	printf("Multicast Address: %s\n", st->multicast_address);
	printf("Data port        : %d\n", st->data_port);
	printf("Bit rate         : %d kb/s\n", st->bit_rate);
}

int main(int argc, char * argv[])
{
  int s_tcp; 									/* socket descriptor */
  struct hostent *hp;
  struct sockaddr_in sin_t;   /* socket struct */
  char *host;
  struct station_info stat1,stat2;
  struct site_info site1,site2;

  if (argc==2) {
    host = argv[1];
  }
  else {
    fprintf(stderr, "usage: client host\n");
    exit(1);
  }
  printf("\n Host: %s\n\n",host);
  /* translate host name into peer's IP address */
  hp = gethostbyname(host);
  if (!hp) {
    fprintf(stderr, "client: unknown host: %s\n", host);
    exit(1);
  }
  else
    printf("Client's remote host: %s\n", argv[1]);

  /* build address data structure for TCP*/
  memset((char *)&sin_t, 0, sizeof(sin_t));
  sin_t.sin_family = AF_INET;
  memcpy((char *)&sin_t.sin_addr, hp->h_addr_list[0], hp->h_length);
  sin_t.sin_port = htons(MC_PORT);

  // Create a TCP socket
  if ((s_tcp = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client TCP: socket");
    exit(1);
  }
  else
    printf("TCP side socket created.\n");

  if (connect(s_tcp, (struct sockaddr *)&sin_t, sizeof(sin_t)) < 0)
    {
      perror("client: connect");
      close(s_tcp);
      exit(1);
    }
  else
    printf("Client connected in tcp.\n");

  if((send(s_tcp, "Start\n", strlen("Start\n")+1, 0)) <0)
    printf("\nclient not ready to receive\n");
  else
    printf("\nstart send successfully\n");

  //Reset
  bzero(&stat1,sizeof(stat1));
  bzero(&stat2,sizeof(stat2));
  bzero(&site1,sizeof(site1));
  bzero(&site2,sizeof(site2));

  //Receive site info for station 1 & 2, then station info for 1 & 2
  //(server sends exactly sizeof(struct) bytes for each, in this order)
  if (recv_all(s_tcp, &site1, sizeof(site1)) < 0 ||
      recv_all(s_tcp, &site2, sizeof(site2)) < 0 ||
      recv_all(s_tcp, &stat1, sizeof(stat1)) < 0 ||
      recv_all(s_tcp, &stat2, sizeof(stat2)) < 0)
  {
    fprintf(stderr, "client: did not receive complete station info from server\n");
    close(s_tcp);
    exit(1);
  }
  /* make sure strings from the network are terminated */
  site1.site_name[sizeof(site1.site_name)-1] = '\0';
  site1.site_desc[sizeof(site1.site_desc)-1] = '\0';
  site2.site_name[sizeof(site2.site_name)-1] = '\0';
  site2.site_desc[sizeof(site2.site_desc)-1] = '\0';
  stat1.station_name[sizeof(stat1.station_name)-1] = '\0';
  stat1.multicast_address[sizeof(stat1.multicast_address)-1] = '\0';
  stat2.station_name[sizeof(stat2.station_name)-1] = '\0';
  stat2.multicast_address[sizeof(stat2.multicast_address)-1] = '\0';

  printf("\n-------------------------------------------------------\n");
  printf("For station 1 site info\n");
  printf("\nSite name: %s",site1.site_name);
  printf("\nSite description: %s\n",site1.site_desc);

  printf("\n-------------------------------------------------------\n");
  printf("For station 2 site info\n");
  printf("\nSite name: %s",site2.site_name);
  printf("\nSite description: %s\n",site2.site_desc);

  print_station(&stat1);
  print_station(&stat2);
  printf("-------------------------------------------------------\n");

  close(s_tcp);		//done with the TCP connection

  //GUI for station list
  gtk_init (&argc, &argv);
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *lab;

  gtk_window_set_title (GTK_WINDOW (window), "Welcome to Television!");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  lab = gtk_label_new ("Hello!!");
  grid = gtk_grid_new ();

  gtk_container_add (GTK_CONTAINER (window), grid);

  /* use the addresses the server told us instead of hard-coding them */
  bd1.label = GTK_LABEL(lab);
  bd1.number = 1;
  snprintf(bd1.address, sizeof(bd1.address), "%s", stat1.multicast_address);
  bd2.label = GTK_LABEL(lab);
  bd2.number = 2;
  snprintf(bd2.address, sizeof(bd2.address), "%s", stat2.multicast_address);

  button = gtk_button_new_with_label ("F.R.I.E.N.D.S");
  g_signal_connect (button, "clicked", G_CALLBACK (on_station), &bd1);	//station 1
  gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);

  button = gtk_button_new_with_label ("H.I.M.Y.M");
  g_signal_connect (button, "clicked", G_CALLBACK (on_station), &bd2);	//station 2
  gtk_grid_attach (GTK_GRID (grid), button, 1, 0, 1, 1);

  button = gtk_button_new_with_label ("EXIT");
  g_signal_connect (button, "clicked", G_CALLBACK (on_exit_clicked), NULL);	//exit
  gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 2, 1);

  gtk_grid_attach (GTK_GRID(grid), lab, 0, 4, 4, 1);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}

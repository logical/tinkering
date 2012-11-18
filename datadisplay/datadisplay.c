//gcc -Wall -export-dynamic -g datadisplay.c -o datadisplay `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0 `
//#define CV_NO_BACKWARD_COMPATIBILITY
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <glib/gprintf.h>


GtkWidget *video,*filedialog;
GdkPixbuf *grid;
double volts,trig,chron,off;
#define UI_FILE "ui.ui"
#define scope_width 640
#define scope_height 480
#define FRAME_DELAY 50
#define sample_size 255
#define resolution 4
int offsety=scope_height/2;
int port; /* File descriptor for the port */
unsigned char signal_data[sample_size];


void getpacket(void);
	
GtkWidget* create_window (void)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;
	
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	gtk_builder_connect_signals (builder, NULL);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	video = GTK_WIDGET (gtk_builder_get_object (builder, "drawingarea1"));
	gtk_widget_set_size_request (video, scope_width, scope_height);

	GtkRange *volts_per_div=(GtkRange *)GTK_WIDGET (gtk_builder_get_object (builder, "vscale1"));
	GtkAdjustment *vpdr=gtk_range_get_adjustment(volts_per_div);
	gtk_adjustment_set_lower(vpdr,0.1);
	gtk_adjustment_set_upper(vpdr,5.0);

	GtkRange *trigger=(GtkRange *)GTK_WIDGET (gtk_builder_get_object (builder, "vscale2"));
	GtkAdjustment *tr=gtk_range_get_adjustment(trigger);
	gtk_adjustment_set_lower(tr,0.1);
	gtk_adjustment_set_upper(tr,5.0);


	GtkRange *time_per_div=(GtkRange *)GTK_WIDGET (gtk_builder_get_object (builder, "hscale1"));
	GtkAdjustment *tpdr=gtk_range_get_adjustment(time_per_div);
	gtk_adjustment_set_lower(tpdr,0);
	gtk_adjustment_set_upper(tpdr,200);

	GtkRange *offset=(GtkRange *)GTK_WIDGET (gtk_builder_get_object (builder, "vscale3"));
	GtkAdjustment *or=gtk_range_get_adjustment(offset);
	gtk_adjustment_set_lower(or,0);
	gtk_adjustment_set_upper(or,scope_height);
	
	g_object_unref (builder);
	
	return window;
}
void voltsperdivcb( GtkWidget *widget , gpointer data){
	GtkAdjustment *adjust=gtk_range_get_adjustment((GtkRange *)widget);
    volts=gtk_adjustment_get_value(adjust);
}

void triggercb( GtkWidget *widget , gpointer data){
	GtkAdjustment *adjust=gtk_range_get_adjustment((GtkRange *)widget);
	trig=gtk_adjustment_get_value(adjust);
}
void timeperdivcb( GtkWidget *widget , gpointer data){
	GtkAdjustment *adjust=gtk_range_get_adjustment((GtkRange *)widget);
	chron=gtk_adjustment_get_value(adjust);
}
void offsetcb( GtkWidget *widget , gpointer data){
	GtkAdjustment *adjust=gtk_range_get_adjustment((GtkRange *)widget);
	off=gtk_adjustment_get_value(adjust);
}

void destroy (GtkWidget *widget, gpointer data)
{
    if (port> 0)
    {
        close(port);
    }
	gtk_main_quit ();
    
}

static gint
timeout (gpointer data)
{
    int x1=0;
    int x2=resolution;
    int y1=scope_height-off;
    int y2=scope_height-off;
    //read signal_data
    float d =(16.0*(5.0/volts))/sample_size;
    printf("multiplier %f \n",d);

    getpacket();
    gdk_draw_pixbuf(video->window,
				    video->style->fg_gc[GTK_STATE_NORMAL],
				    grid,
                    0,0,
				    0,0,
				    scope_width,
				    scope_height,
				    GDK_RGB_DITHER_NORMAL,
				    0,
				    0
				    );

    int s=scope_width/resolution;
    for(int i=0;i<s;i++){
        y2=scope_height-off-signal_data[i]*d;
        gdk_draw_line(video->window,video->style->white_gc,x1,y1,x2,y2);
        x1+=resolution;
        x2+=resolution;
        y1=y2;
    }

    return TRUE;
}


void getpacket(void){
    int n = write(port, "S", 1);
    //set signal data to 0
    for(int i=0;i<sample_size;i++)signal_data[i]=0;
    if (n < 0)fputs("write() of S failed!\n", stderr);
    int result = read(port,signal_data,sample_size);
    if(result<0)printf("read encountered an error \n");
    else if(result<sample_size)printf("read %d of %d bytes\n",result,sample_size);

}

void open_port(void){

    port = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (port == -1){
        perror("open_port: Unable to open /dev/ttyACM0 - ");
 //       exit(1);
    }
    else
    fcntl(port, F_SETFL, 0);
}

int main( int   argc,char *argv[]){
	g_type_init();
	GtkWidget *window;
    open_port();
	gtk_init(&argc, &argv);
    grid = gdk_pixbuf_new_from_file("./grid.png",NULL);
	window = create_window ();
    g_timeout_add( FRAME_DELAY, ( GSourceFunc )timeout, NULL );
    gtk_widget_show (window);
	gtk_main ();
	return 0;

}




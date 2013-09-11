//gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` lognotifier.c -o lognotifier
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <regex.h>

#define MAX_BUF 1024
#define MAX_ERROR_MSG 1024
#define LOGNAME "/var/log/messages"
#define REGEXTEXT "DENY"

GdkPixbuf *okiconpixbuf;
GdkPixbuf *hiticonpixbuf;
static GtkWidget *my_menu = NULL;
GtkWidget *logwindow;
GtkWidget *logevents;
GtkWidget *settingswindow;
GtkWidget *settingslogfile;
GtkWidget *settingsfilter;
GtkWidget *settingsset;
GtkRequisition windowsize;

static GtkStatusIcon *status_icon = NULL;
FILE *logfile;

regex_t pattern;
extern const char * firewall_xpm[];
extern const char * firewall2_xpm[];
char log_file_name[MAX_BUF]=LOGNAME;
char regex_filter_text[MAX_BUF]=REGEXTEXT;

void getsettings(void){
    FILE *settingsfile;
    char settingspath[MAX_BUF] ="";
    sprintf(settingspath,"%s%s",getenv("HOME"),"/.lognotifier");
    settingsfile=fopen(settingspath,"rw+");
    fgets(log_file_name,MAX_BUF,settingsfile);
    fgets(regex_filter_text,MAX_BUF,settingsfile);
//remove last character
    log_file_name[strlen(log_file_name)-1]='\0';
    regex_filter_text[strlen(regex_filter_text)-1]='\0';
    fclose(settingsfile);  
   regcomp(&pattern, regex_filter_text, 0);      
   logfile=fopen(log_file_name,"r");
   fseek(logfile, 0L, SEEK_END);

  
}

void savesettings(char *logname,char *regextext){
    FILE *settingsfile;
    char settingspath[MAX_BUF] ="";
    sprintf(settingspath,"%s%s",getenv("HOME"),"/.lognotifier");
    settingsfile=fopen(settingspath,"w+");
    fprintf(settingsfile,"%s\n%s\n",logname,regextext);
    fclose(settingsfile);  
    sprintf(log_file_name,"%s",logname);
    sprintf(regex_filter_text,"%s",regextext);
}

/*
 * static int compile_regex (regex_t * r, const char * regex_text)
{
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
    if (status != 0) {
	char error_message[MAX_ERROR_MSG];
	regerror (status, r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                 regex_text, error_message);
        return 1;
    }
    return 0;
}

*/

static void 
destroy(GtkWidget *widget,
		gpointer data)
{
	gtk_main_quit ();
}

static gint
checklog (gpointer data)
{
    GtkTextBuffer *buffer=gtk_text_view_get_buffer (GTK_TEXT_VIEW (logevents));;
    char buf[MAX_BUF]="";
    fgets(buf, MAX_BUF,logfile);
    if(strlen(buf)){
      if(!regexec(&pattern,buf,0,NULL,0)){
	GtkTextIter enditer;
	gtk_status_icon_set_from_pixbuf(status_icon,hiticonpixbuf);
	gtk_text_buffer_get_end_iter(buffer,&enditer);
	gtk_text_buffer_insert(buffer,&enditer, buf, strlen(buf));
	gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(status_icon), "Firewall is warm");
      }
    } 
      return TRUE;
  
}



static void
settings_activate (GtkWidget *widget,
		gpointer user_data)
{
  gtk_entry_set_text(GTK_ENTRY(settingslogfile),log_file_name);
  gtk_entry_set_text(GTK_ENTRY(settingsfilter),regex_filter_text);
  gtk_widget_show_all(settingswindow);
}

static void
settings_save (GtkWidget *widget,
		gpointer user_data)
{
  gtk_widget_hide(settingswindow);
  const gchar* logname=gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(settingslogfile)));
  const gchar* regextext=gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(settingsfilter)));
//  g_printf("%s%s",logname,regextext);	
  fclose(logfile);
  logfile=fopen(logname,"r");
  fseek(logfile, 0L, SEEK_END);
  regfree(&pattern);
  regcomp(&pattern, regex_filter_text, 0);      

  //logfile=fopen(log_file_name,"r");
  savesettings((char*)logname,(char*)regextext);
  
  
  
 
}

static void
view_events_activate (GtkWidget *widget,
		gpointer user_data)
{
	gtk_widget_show_all(logwindow);
	gtk_status_icon_set_from_pixbuf(status_icon,okiconpixbuf);
	gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(status_icon), "Firewall is cold");
}

static void 
popup(GtkStatusIcon *status_icon,
		guint button,
		guint activate_time,
		gpointer user_data)
{
	g_debug("'popup-menu' signal triggered");

	if (!my_menu)
	{
		GtkWidget *item;
		my_menu = gtk_menu_new();

		item = gtk_menu_item_new_with_label("Settings");
		gtk_menu_shell_append(GTK_MENU_SHELL(my_menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(settings_activate),
				GUINT_TO_POINTER(TRUE));

		item = gtk_menu_item_new_with_label("Events");
		gtk_menu_shell_append(GTK_MENU_SHELL(my_menu), item);
		g_signal_connect (G_OBJECT(item), "activate",
				G_CALLBACK(view_events_activate), 
				GUINT_TO_POINTER(FALSE));
		
		item = gtk_menu_item_new_with_label("Quit");
		gtk_menu_shell_append(GTK_MENU_SHELL(my_menu), item);
		g_signal_connect (G_OBJECT(item), "activate",
				G_CALLBACK(destroy), 
				NULL);
	}
	
	gtk_widget_show_all(my_menu);

	gtk_menu_popup(GTK_MENU(my_menu),
			NULL,
			NULL,
			gtk_status_icon_position_menu,
			status_icon,
			button,
			activate_time);
}


int main( int argc, 
	  char* argv[] )
{

  gtk_init( &argc, &argv );
    getsettings();
    okiconpixbuf = gdk_pixbuf_new_from_xpm_data( firewall_xpm);
    hiticonpixbuf = gdk_pixbuf_new_from_xpm_data( firewall2_xpm);
    status_icon = gtk_status_icon_new_from_pixbuf(okiconpixbuf);
    
    //create log window  
    logwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (logwindow), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    gtk_window_set_default_size(GTK_WINDOW(logwindow),128,64);
    GtkWidget *scroller=gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add (GTK_CONTAINER (logwindow), scroller);
    logevents=gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(logevents),GTK_WRAP_WORD);
    gtk_container_add (GTK_CONTAINER (scroller), logevents);

    

    //create settings window
    settingswindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (settingswindow), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    gtk_window_set_default_size(GTK_WINDOW(settingswindow),128,64);
    GtkWidget *settingsvbox=gtk_vbox_new(TRUE,10);
    settingslogfile=gtk_entry_new();
    settingsfilter=gtk_entry_new();
    settingsset=gtk_button_new_with_label("ok");
    gtk_container_add (GTK_CONTAINER (settingswindow), settingsvbox);
    gtk_container_add (GTK_CONTAINER (settingsvbox),gtk_label_new("Name of the log you want to watch\nDefault = /var/log/messages"));
    gtk_container_add (GTK_CONTAINER (settingsvbox), settingslogfile);
    gtk_container_add (GTK_CONTAINER (settingsvbox),gtk_label_new("Regex Filter to apply to entry \n Default = 'DENY' "));
    gtk_container_add (GTK_CONTAINER (settingsvbox), settingsfilter);
    gtk_container_add (GTK_CONTAINER (settingsvbox), settingsset);
    g_signal_connect (G_OBJECT (settingsset), "clicked", G_CALLBACK (settings_save), NULL);
 
    
	gtk_status_icon_set_visible(status_icon, TRUE);	
	g_debug("embedded: %s", gtk_status_icon_is_embedded(status_icon) ? "yes" : "no");

	gtk_status_icon_set_tooltip_text(status_icon, "Firewall is cold");

	/* Connect signals */
	g_signal_connect (G_OBJECT (status_icon), "popup-menu",
			  G_CALLBACK (popup), NULL);

	g_signal_connect (G_OBJECT (status_icon), "activate",
			  G_CALLBACK (view_events_activate), NULL);

	g_timeout_add_seconds(1,( GSourceFunc )checklog,NULL);

	gtk_main();

   fclose(logfile);
   regfree(&pattern);   
	return 0;
}

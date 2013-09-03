CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DGDK_DISABLE_DEPRECATED 	\
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED

lognotifier: lognotifier.c firewall.c firewall2.c
	$(CC) $(CFLAGS) lognotifier.c firewall.c firewall2.c -o lognotifier `pkg-config gtk+-2.0 --cflags --libs`

clean: 
	rm -f *.o lognotifier

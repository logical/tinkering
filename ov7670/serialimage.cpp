//g++ -export-dynamic -g serialimage.cpp -o serialimage -lm  -lpthread `pkg-config --cflags opencv` `pkg-config  --cflags --libs opencv`

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <opencv2/core/core.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
using namespace cv;
using namespace std;


#define image_width 120
#define image_height 80
#define sample_size (image_width*image_height)

uint8_t signal_data[sample_size];
int port; /* File descriptor for the port */


void open_port(void){

    port = open("/dev/ttyACM0", O_RDWR | O_NOCTTY );
    if (port == -1){
        perror("open_port: Unable to open /dev/ttyACM0 - ");
        exit(1);
    }
    else
    fcntl(port, F_SETFL, 0);
}

Mat getpacket(void){
	int result=0;	
    memset(signal_data,0,sample_size);
    int n = write(port, "a", 1);
	
    if (n < 0)fputs("write() of S failed!\n", stderr);
	for (int j=0; j<image_height; j++) {
      //sprintf(str, "%02x", img_buf[i][j]); USBS.print(str);
	    result += read(port,&signal_data[result],image_width);
		if(result<0)printf("read encountered an error \n");
		else if(result<image_width){
			printf("read %d of %d bytes\n",result,sample_size);
		}
      
  	}

    Mat image(Size(image_width, image_height), CV_8UC1, signal_data, Mat::AUTO_STEP);
	return image;
}

int main( int argc, char** argv )
{
	Mat image;
    open_port();
    namedWindow( "Display window", WINDOW_AUTOSIZE ); // Create a window for display.
	/*load data from serial*/
	while(1){	
		image=getpacket();
		if( image.empty() )                      // Check for invalid input
		{
		    cout <<  "Could not open or find the image" << std::endl ;
		    return -1;
		}

	    imshow( "Display window", image );                // Show our image inside it.
    	if(waitKey(1)>0)break; // Wait for a keystroke in the window
	}
	close(port);
    return 0;
}

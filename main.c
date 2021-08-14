#include <stdio.h>				//
#include <stdlib.h>				//
#include <sys/ioctl.h>			// interface with V4L2 
#include <getopt.h>				// get command line options
#include <fcntl.h>				// open() function
#include <errno.h>				// Error handling
#include <sys/mman.h>			// mmap() function
#include <unistd.h>				// sleep() function 
#include <stdint.h>				// uint8_t for the buffer

#include <linux/videodev2.h>	// The main V4L2 header (camera api functions)
#include "camera.h"				// custom camera functions to make code cleaner

#define PXWIDTH 320				// Capture resolution, change this to match the output 
#define PXHEIGHT 240			// of `v4l2-ctl -d <n> --all | grep Bounds` if it fails

// Global variables for options only
int totalpixels = PXWIDTH*PXHEIGHT, minbright = 1, time = 5, fade = 500, oneshot_flag, help_flag;
float multiplier = 1;
char *device;

float getbacklight() {
	FILE *pipe = popen("xbacklight -get", "r");
	char *temp = malloc(16);
	fscanf(pipe, "%s", temp);
	float retval = atof(temp);
	pclose(pipe);
	return retval;
}

void setbacklight(float value, int time) {
	char cmd[64];
	sprintf(cmd, "xbacklight -set %f -time %i", value, time);
	system(cmd);
}

// Get brightness of image from YUYV buffer
float getbrightness(uint8_t* buffer) {
	int bignumber = 0;
	float returnval;
	for (int i = 0; i < totalpixels; i+=2) {
		bignumber+=buffer[i];
	}
	returnval = (bignumber/totalpixels - 8 + minbright) * multiplier; 
	if (returnval <= 0) {
		returnval = minbright;
	}
	return returnval;
}

void help() {
	printf("\
usage: xbacklight-auto [options]\n\
  options:\n\
    -h | --help                    display this help\n\
    -d | --device </dev/videoN>    video device to capture from (default /dev/video0)\n\
    -f | --fade <milliseconds>     time to fade between brightnesses (defaults 500)\n\
    -m | --minimum <percentage>    minimum possible brightness\n\
    -o | --oneshot                 run once and exit\n\
    -t | --time <seconds>          time between samples/sets (works best when ≥1)\n\
    -x | --multiplier              multiplier on the set brightness (defaults to 1)\n\
");
}

void getoptions(int argc, char** argv) {
	int c;
	for (;;) {
		static struct option long_options[] = 
		{
			{"help"			, no_argument, &help_flag	, 1},
			{"oneshot"		, no_argument, &oneshot_flag, 1},
			{"minimum"		, required_argument, 0, 'm'},
			{"multiplier"	, required_argument, 0, 'x'},
			{"time"			, required_argument, 0, 't'},
			{"device"		, required_argument, 0, 'd'},
			{"fade"			, required_argument, 0, 'f'},
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "m:t:x:ohf:", long_options, &option_index);

		if (c == -1) { break; }

		switch(c) {
			case 0:		//do nothing
				break;
			case 't': // time
				time = atoi(optarg);
				break;
			case 'x': //multiplier
				multiplier *=atof(optarg);
				break;
			case 'm': //minbright	
				minbright = atoi(optarg);
				break;
			case 'o': // oneshot
				oneshot_flag = 1;
				break;
			case 'd': // device
				device = optarg;
				break;
			case 'f': // fade
				fade = atoi(optarg);
				break;
			case 'h': // help 
				help_flag = 1;
				break;
			default:
				abort();
		}
	}
}

int main(int argc, char **argv) {
	float brightness, xstate, offset = 1;
	uint8_t *buffer;
	int buf_addr;

	// Get options
	getoptions(argc, argv);

	if (help_flag == 1) {
		help();
		return 0;
	}

	if (oneshot_flag == 1) {
		time = 0;
	}

	if (device == NULL) {
		device = "/dev/video0";
	}
	
	int fd = camera_init(device, PXWIDTH, PXHEIGHT);

	// main loop
	do {
		struct getbuf_return mybuffer = get_buffer(fd);
		if (mybuffer.code != 0) {
			return 1;
		} else {
			buffer = mybuffer.buffer;
		}

		// get and set brightness
		brightness = getbrightness(buffer)*offset;
		setbacklight(brightness, fade);

		sleep(time);

		// get offset as a multiplier
		xstate = getbacklight();
		offset = xstate/(brightness/offset);
	}	
	while (oneshot_flag != 1);

	// Never gets used lmao
	return 0;
}

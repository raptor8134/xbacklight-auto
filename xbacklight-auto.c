#include <stdio.h>				//
#include <stdlib.h>				//
#include <fcntl.h>				// open() function
#include <sys/ioctl.h>			// Error handling
#include <errno.h>				// ^^
#include <sys/mman.h>			// mmap() function
#include <unistd.h>				// sleep() function 
#include <stdint.h>				// uint8_t for the buffer
#include <string.h>				// memset() function

#include <linux/videodev2.h>	// The main V4L2 header

#define VDEV "/dev/video0"		// The video device you want to capture from 
#define PXWIDTH 320				// Capture resolution, change this to match 
#define PXHEIGHT 240			// the output of `v4l2-ctl -d <n> --all | grep Bounds`

// initialize the buffer pointer and buffer iterator
uint8_t *buffer;
int totalpixels = PXWIDTH*PXHEIGHT;

// find out if we have any screw-ups while doing ioctl()
static int xioctl(int fd, int request, void *arg) {
	int r;
	do r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

int getbrightness(uint8_t* buffer) {
	// TODO get the brightness from the pointer
	// Note: 4 bytes = 2 pixels
	int n, returnval, bignumber = 0;
	for (int i=0; i<totalpixels; i+=2) {
		bignumber+=buffer[i];
	}
	returnval = bignumber/totalpixels;
	printf("%i\n", returnval);
	return returnval; // lets not break the program in testing
}

int main(int argc, char **argv) { //TODO add command line options
	char cmd[64];
	int brightness;

	// Open the video device
	int fd = open(VDEV, O_RDWR);
	if (fd == -1) {
		perror("Error opening device");
		return 1;
	}

	// Get the capabilities of the camera
	struct v4l2_capability caps;
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps)) {
		perror("Querying capabilities");
		return 1;
	}
	
	// Set the capture format 
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = PXWIDTH;
	fmt.fmt.pix.height = PXHEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
		perror("Error setting pixel format");
		return 1;
	}

	// Request buffer
	struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        perror("Requesting buffer");
        return 1;
    }

	// main loop
	for (;;) {
		// Query buffer(s) 
	    struct v4l2_buffer buf;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = 0; 
		if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
			perror("Querying buffer");
			return 1;
		}
		buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		// Consoom frame and get excited for next frame
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
			perror("Starting capture");
			return 1;
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		struct timeval tv = {0};
		tv.tv_sec = 2;
		int r = select(fd+1, &fds, NULL, NULL, &tv);

		if(-1 == r) {
			perror("Waiting for frame");
			return 1;
	    }

    	if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			perror("Retrieving frame");
			return 1;
    	}

		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &buf.type)) {
			perror("Stopping capture");
		}

		/* TODO
		 * These are sufficient for now, but I want to add a more
		 * native method for changing the xbacklight state later
		 */
		
		brightness = getbrightness(buffer);
		sprintf(cmd, "xbacklight -set %i%%", brightness);
		system(cmd);

		sleep(3);
	}
	// Never gets used lmao
	return 0;
}

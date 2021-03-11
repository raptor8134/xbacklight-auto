#include <stdio.h>				//
#include <stdlib.h>				//
#include <sys/param.h>			// MIN()/MAX() functions
#include <fcntl.h>				// open() function
#include <sys/ioctl.h>			// Error handling
#include <errno.h>				// ^^
#include <sys/mman.h>			// mmap() function
#include <unistd.h>				// sleep() function 
#include <stdint.h>				// uint8_t for the buffer

#include <linux/videodev2.h>	// The main V4L2 header

#define VDEV "/dev/video0"		// The video device you want to capture from 
#define PXWIDTH 320				// Capture resolution, change this to match 
#define PXHEIGHT 240			// the output of `v4l2-ctl -d <n> --all | grep Bounds`

// initialize the buffer pointer
uint8_t *buffer;	

// find out if we have any screw-ups
static int xioctl(int fd, int request, void *arg) {
	int r;
	do r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

int main(int argc, char **argv) { //TODO add command line options
	// Things we use later
	char cmd[50];
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

	// Query buffer 
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        perror("Querying buffer");
        return 1;
    }
    buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

	// Handle a capture error before the loop starts
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
		perror("Error in start capture");
		return 1;
	}

	// main loop
	while (1==1) {
		// Get a frame
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

		// This guy is causing a hangup, dunno why
		// Ok i think its the thingy in the while loop in xioctl
    	if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			perror("Retrieving frame");
			return 1;
    	}

		//TODO
		// Take a YUYV format string and make it into an average brightness

		brightness = 50;

		// These are sufficient for now, but I want to add a more
		// native method for changing the xbacklight state later
		sprintf(cmd, "xbacklight -set %i%%", brightness);
		system(cmd);

		sleep(10);
	}

	return 0;
}

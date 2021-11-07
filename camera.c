/* Big thanks to https://gist.github.com/maxlapshin/1253534 for most of the V4L2 stuff here*/
#include <sys/mman.h>			// mmap() function
#include <sys/ioctl.h>			// interface with V4L2 
#include <fcntl.h>				// open() function
#include <errno.h>				// Error handling
#include <stdio.h>				//
#include <stdint.h>				// uint8_t for the buffer

#include <linux/videodev2.h>	// The main V4L2 header (camera api functions)
#include "camera.h"

// find out if we have any screw-ups while doing ioctl()
static int xioctl(int fd, int request, void *arg) {
	int r;
	do r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

int camera_init(char* device, int PXWIDTH, int PXHEIGHT) {
	// Open the video device
	int fd = open(device, O_RDWR);
	if (fd == -1) {
		perror("Error opening device");
		return -1;
	}

	// Get the capabilities of the camera
	struct v4l2_capability caps;
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps)) {
		perror("Querying capabilities");
		return -1;
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
		return -1;
	}

	// Request buffer
	struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        perror("Requesting buffer");
        return -1;
    }

	return fd;
}

struct getbuf_return get_buffer(int fd) {
	struct getbuf_return return_struct;
		return_struct.code = 0;

	// Query buffer 
	struct v4l2_buffer buf;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0; 
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
		perror("Querying buffer");
		return_struct.code = 1;
	}
	return_struct.buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE,\
		MAP_SHARED, fd, buf.m.offset);

	return_struct.length = buf.length;

	// Consoom frame and get excited for next frame
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
		perror("Starting capture");
		return_struct.code = 1;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval tv = {0};
	tv.tv_sec = 2;
	int r = select(fd+1, &fds, NULL, NULL, &tv);

	if(-1 == r) {
		perror("Waiting for frame");
		return_struct.code = 1;
	}

	if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		perror("Retrieving frame");
		return_struct.code = 1;
	}

	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &buf.type)) {
		perror("Stopping capture");
	}

	return return_struct;
}

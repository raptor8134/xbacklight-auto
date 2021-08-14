#ifndef _MYCAMERA
#define _MYCAMERA	1

struct getbuf_return {
	uint8_t	*buffer;
	int 	code;
};

extern int camera_init(char* device, int PXWIDTH, int PXHEIGHT);

extern struct getbuf_return get_buffer(int fd);

#endif

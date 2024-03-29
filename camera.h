#ifndef __XBA_CAMERA_H__
#define __XBA_CAMERA_H__

struct getbuf_return {
    uint8_t *buffer;
    int code;
    int length;
};

extern int camera_init(char *device, int PXWIDTH, int PXHEIGHT);
extern int camera_close(int fd);

extern struct getbuf_return get_buffer(int fd);

#endif //__XBA_CAMERA_H__

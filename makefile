CC=gcc
CFLAGS=-Wall -Wpedantic.
DEPS= stdio.h stdlib.h fcntl.h sys/ioctl.h errno.h sys/mman.h unistd.h stdint.h string.h linux/videodev2.h

all:
	$(CC) -o xbacklight-auto xbacklight-auto.c
test:
	@ for header in $(DEPS); do printf "found "; ls /usr/include/$$header; done
	@ echo "All dependencies satisfied." 
clean: 
	@rm xbacklight-auto 
install:
	install ./xbacklight-auto /usr/bin/xbacklight-auto
uninstall:
	rm /usr/bin/xbacklight-auto

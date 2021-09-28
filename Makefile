NAME=xbacklight-auto
CC=cc
LD=ld
CFLAGS=
PREFIX=/usr/bin/
DESTDIR=
DEPS= stdio.h stdlib.h sys/ioctl.h getopt.h fcntl.h errno.h sys/mman.h unistd.h stdint.h string.h linux/videodev2.h
all:
	$(CC) -c camera.c $(CFLAGS)
	$(CC) -c main.c $(CFLAGS)
	$(CC) -o $(NAME) main.o camera.o
test:
	@ for header in $(DEPS); do printf "found "; ls /usr/include/$$header; done
	@ echo "All dependencies satisfied." 
clean: 
	rm *.o
	rm $(NAME)
install:
	install $(NAME) $(DESTDIR)/$(PREFIX)/$(NAME)
uninstall:
	rm $(DESTDIR)/$(PREFIX)/$(NAME)

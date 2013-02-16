CC := gcc
CFLAGS := -O2 -Wall
PREFIX := /usr/local

wold: wold.c
	${CC} ${CFLAGS} wold.c -o wold

all: wold

install: all
	install -D wold ${DESTDIR}${PREFIX}/bin/wold

clean:
	rm -f wold

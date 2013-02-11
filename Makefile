CC = gcc
CFLAGS = -O2 -Wall

all:
	${CC} ${CFLAGS} wold.c -o wold

clean:
	rm -f wold

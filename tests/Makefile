CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes -O3 -I../include
LIBS=../src/bitmap.o ../src/simplefs.o ../src/disk_driver.o
CC=cc
AR=ar

SRCS=$(wildcard *.c)

BINS=$(SRCS:.c=)

%: %.c
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

.phony: clean all

all: $(BINS) 

clean:
	rm -rf *.o *~  $(BINS) *.disk

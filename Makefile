CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes -O3
LIBS= 
CC=cc
CXX=c++
AR=ar


BINS= simplefs_test

OBJS = bitmap.o simplefs.o disk_driver.o

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

simplefs_test: simplefs_test.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)

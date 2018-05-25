CCOPTS= -fPIC -Wall -g -std=gnu99 -Wstrict-prototypes -O3
LIBS= 
CC=cc
AR=ar

OBJS = bitmap.o simplefs.o disk_driver.o

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.PHONY: clean all tests

all: shared

shared: $(OBJS) 
	$(CC) $(CCOPTS) -shared -o libsimplefs.so $^ $(LIBS)

tests: all
	cd tests && make
	cd tests && python run_tests.py

clean:
	rm -rf *.o *~ $(BINS) *.so
	cd tests && make clean

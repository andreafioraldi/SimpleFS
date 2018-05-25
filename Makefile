CCOPTS= -fPIC -Wall -g -std=gnu99 -Wstrict-prototypes -O3 -Iinclude
LIBS= 
CC=cc
AR=ar

OBJS = src/bitmap.o src/simplefs.o src/disk_driver.o

HEADERS=include/bitmap.h\
	include/disk_driver.h\
	include/simplefs.h

%.o: %.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.PHONY: clean all tests

all: shared

shared: $(OBJS) 
	$(CC) $(CCOPTS) -shared -o libsimplefs.so $^ $(LIBS)

tests: all
	cd tests && make
	cd tests && python run_tests.py

clean:
	rm -rf *.o *~ $(BINS) *.so src/*.o
	cd tests && make clean

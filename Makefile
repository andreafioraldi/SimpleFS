CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
CXXOPTS= -Wall -g -std=c++0x
LIBS= 
CC=cc
CXX=c++
AR=ar


BINS= simplefs_shell simplefs_test

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

simplefs_shell: simplefs_shell.cpp $(OBJS)
	$(CXX) $(CXXOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)

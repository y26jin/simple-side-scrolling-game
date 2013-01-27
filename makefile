CC = g++
CFLAGS = -g
INCLUDES = -I/usr/X11R6/include
LFLAGS = -L/usr/X11R6/lib
LIBS = -lX11
SRC = main.cpp
OBJS = $(SRC:.c=.o)
MAIN = main

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)


.PHONY: run clean

run:	$(MAIN)
	$(shell ./main)

clean: 
	$(RM) -rf *.o *~ $(MAIN)
SRCS=IT8951.c main.c
CC=gcc
TARGET=IT8951
CFLAGS ?= -O3 -Wall

$(TARGET):$(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -lbcm2835 -lpng -lpthread
	
clean:
	rm -f $(TARGET)

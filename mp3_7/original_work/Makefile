# Makefile for TFTP server project

CC = gcc
CFLAGS = -Wall
TARGET = server
SRCS = server.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

CC = gcc
CFLAGS = -Wall
LDFLAGS =
LDLIBS = -lpthread -lm

TARGET = main
SRCS = main.c FCFS_threads.c SJF_threads.c Priority_threads.c RoundRobin_threads.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)

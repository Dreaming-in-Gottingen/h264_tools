CC = gcc

SOURCE_ROOT = $(shell pwd)

CFLAGS  :=
LDFLAGS := -L. libx264.so.161
INCLUDE := -I .

TARGET  := x264_demo

all: $(TARGET)
SRCS = example.c

OBJS = $(patsubst %.c, %.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@

clean:
	@rm -f *.o $(TARGET)

.PHONY: clean

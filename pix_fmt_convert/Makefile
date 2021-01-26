CC = gcc

SOURCE_ROOT = $(shell pwd)

CFLAGS  :=
LDFLAGS :=
INCLUDE :=

TARGET  := rgba2yuv

all: $(TARGET)
SRCS = rgba2yuv.c

OBJS = $(patsubst %.c, %.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@

clean:
	@rm -f *.o $(TARGET)

.PHONY: clean

CC = gcc

SOURCE_ROOT = $(shell pwd)

CFLAGS  :=
LDFLAGS :=
INCLUDE :=

TARGET  := soft_scale

all: $(TARGET)
SRCS = soft_scale.c

OBJS = $(patsubst %.c, %.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@

clean:
	@rm -f *.o $(TARGET)

.PHONY: clean

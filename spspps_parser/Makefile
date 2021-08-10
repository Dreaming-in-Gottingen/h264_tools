CC = g++

SOURCE_ROOT = $(shell pwd)

CPPFLAGS	:=
INCLUDE 	:= -I .

TARGET      := h264_mediadata_detector

all: $(TARGET)
SRCS = h264_mediaData_detector.cpp \
       bs_read.cpp

OBJS = $(patsubst %.cpp, %.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

$(OBJS): %.o:%.cpp
	$(CC) $(CPPFLAGS) $(INCLUDE) -c $^ -o $@

clean:
	@rm -f *.o $(TARGET)

.PHONY: clean

CXX = g++

SOURCE_ROOT = $(shell pwd)

CPPFLAGS        :=
INCLUDE         := -I .

TARGET_BIN      := spspps_parser

CXX_SRCS = $(wildcard *.cpp)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_SRC))

all: $(TARGET_BIN)

$(TARGET_BIN):$(CXX_SRCS)
	$(CXX) $(CPPFLAGS) $(INCLUDE) $^ -o $@

clean:
	rm -f *.o $(TARGET_BIN)

.PHONY: clean

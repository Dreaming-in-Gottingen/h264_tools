SRCS := $(shell find ffmpeg-src -name "*.c" -not -name "*_template.c")

CFLAGS := -I./ffmpeg-src -O2 -w -std=c99 -Wno-implicit-function-declaration -fPIC
LDFLAGS := -lm

OBJS := $(SRCS:%.c=%.o)

EXE_SRC := $(shell find . -name *.c)
EXE_OBJ := $(EXE_SRC:%.c=%.o)

#libffh264dec.so: $(OBJS)
#	$(CC) -shared -o $@ $^ $(LDFLAGS)

LIB := libffh264dec.so
EXE := ffH264DecDemo

$(warning --------------------------------------------------)
$(warning |target lib_name=$(LIB)                 |)
$(warning |target exec_name=$(EXE)                  |)
$(warning --------------------------------------------------)

all := $(EXE)
$(EXE): $(EXE_OBJ) $(LIB)
	$(CC) -o $@ $^
$(LIB): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(LIB) $(EXE) *.o

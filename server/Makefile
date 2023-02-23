SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)
CC := $(CROSS_COMPILE)gcc
CCFLAGS += -Wall -Werror

all: $(TARGET)

$(TARGET) : $(SRC)
	$(CC) $(CCFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f *.o $(TARGET) a.out
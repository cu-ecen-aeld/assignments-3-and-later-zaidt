SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)
CC := $(CROSS_COMPILE)gcc

all: $(TARGET)

$(TARGET) : $(SRC)
	$(CC) $(SRC) -o $(TARGET)

clean:
	rm -f *.o $(TARGET) a.out
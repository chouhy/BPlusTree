TARGET = bplustree
CC = g++
CFLAGS = -g -O2

all: $(TARGET).cpp
	$(CC) $^ $(CFLAGS) -o $(TARGET)
	
clean:
	rm -rf $(TARGET)
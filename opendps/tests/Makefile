CFLAGS = -I. -I.. -Wall

all: 
	gcc -o protocol_test $(CFLAGS) protocol_test.c ../uframe.c ../protocol.c ../crc16.c && ./protocol_test

clean:
	rm -f protocol_test

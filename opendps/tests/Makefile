CFLAGS = -I. -I.. -Wall

all: 
	gcc -o protocol_test $(CFLAGS) protocol_test.c ../uframe.c ../protocol.c ../crc16.c && ./protocol_test
	gcc -m32 -o past_test $(CFLAGS) past_test.c ../past.c && ./past_test

clean:
	rm -f protocol_test past_test

CFLAGS = -I. -I.. -Wall

all: 
	gcc -o protocol_test $(CFLAGS) protocol_test.c ../uframe.c ../protocol.c && ./protocol_test

clean:
	rm -f protocol_test

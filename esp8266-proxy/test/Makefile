packer: packer.c ../hexdump.c ../packer.h
	gcc -o packer packer.c ../hexdump.c -I..
test: packer
	./packer
clean:
	rm -f packer
tcc -nostdlib /usr/lib/crt0.o hello.c -lc -lnosys -o hello.o

tcc -nostdlib /usr/lib/crt0.o hello.c -I/usr/include -lc -lnosys -o hello

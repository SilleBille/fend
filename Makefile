all: fend.c initializer.c initheader.h
	gcc fend.c initializer.c -o fend

clean:
	rm *.o

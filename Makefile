all: fendc.c initializer.c initheader.h
	gcc fend2.c initializer.c -o fend

clean:
	rm *.o

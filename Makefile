# all:
# 	gcc fend.c parseArguments.c -o fend -Wall;

# clean: 
# 	rm -f fend.o*

all: fend2.c initializer.c initheader.h
	gcc fend2.c initializer.c -o fend

clean:
	rm *.o
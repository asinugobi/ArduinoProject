all: read

read:
	c++ -o read ReadUSB.cpp

clean: 
	rm -rf read a.out *.o
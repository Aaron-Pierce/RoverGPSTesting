all: main

main:	
	gcc read_i2c.c -o read_i2c -O -Wall -Wpedantic -li2c
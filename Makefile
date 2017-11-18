CC=gcc
CCFLAGS=-Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv7-a -mtune=arm1176jzf-s -std=c++0x
LDFLAGS=-lrf24-bcm -lrf24network -lstdc++

HomeKitAmp: HomeKitAmp.o
	$(CC) $(CCFLAGS) $(LDFLAGS) $^ -o HomeKitAmp

HomeKitAmp.o: ./RF24/RF24.h ./RF24Network/RF24Network.h

%.o: %.c
	$(CC) $(CCFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f *.o *~
	rm -f main

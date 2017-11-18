#include <cstdlib>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>

/**
* g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
**/
using namespace std;

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);

RF24Network network(radio);

// Address of our node in Octal format (01,021, etc)
const uint16_t amp_node = 01;
const uint16_t rpi_node = 00;

typedef enum { NONE, POWER_UP, POWER_DOWN, UPDATE_STATE }Action;

typedef struct {                 // Structure of our request
	Action actionReq;				 // 'I' => POWER_ON - 'O' => POWER_DOWN - 'S' => UPDATE_STATE
} request_t;

typedef struct {				   //Structure of our answer
	char currentState;			   // ON => 'I', OFF => 'O'
	//char actionStatusCode;	     //OK : 0 - Error : 1
	uint16_t delay;				   //delay in s
} answer_t;


const unsigned long interval = 2000; //ms  // How often to send 'hello world to the other unit

unsigned long last_sent; // When did we last send?

int main(int argc, char** argv)
{
	// Refer to RF24.h or nRF24L01 DS for settings

	radio.begin();

	delay(5);
	network.begin(/*channel*/ 90, /*node address*/ rpi_node);
	radio.printDetails();

	while (1) {
		network.update();
		bool sentOnce = true;
		unsigned long now = millis();              // If it's time to send a message, send it!
		if (now - last_sent >= interval && sentOnce) {
			last_sent = now;

			printf("Sending ..\n");
			request_t request = { POWER_UP };
			RF24NetworkHeader header(/*to node*/ amp_node);
			bool ok = network.write(header, &request, sizeof(request));
			if (ok) {
				sentOnce = false;
				printf("ok.\n");
			}
			else {
				printf("failed.\n");
			}
		}
	}

	return 0;

}

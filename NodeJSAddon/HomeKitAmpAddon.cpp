#include <cstdlib>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>
#include "streaming-worker.h"

using namespace std;

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

class HomeKitAmp : public StreamingWorker {
public:
	HomeKitAmp(Callback *data, Callback *complete, Callback *error_callback, v8::Local<v8::Object> & options)
		: StreamingWorker(data, complete, error_callback) {}

	void Execute(const AsyncProgressWorker::ExecutionProgress& progress) {
		// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
		RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
		RF24Network network(radio);

		while (!closed()) {
			network.update();
			while (network.available()) {     // Check for incoming radio data
				RF24NetworkHeader header;
				answer_t answer;
				network.read(header, &answer, sizeof(answer));

				printf("Received answer with status :");
				printf("Status : %c, delay : %dms", answer.currentState, answer.delay);
				Message tosendStatus("status", std::to_string(answer.currentState));
				Message tosendDelay("delay", std::to_string(answer.delay));
				writeToNode(progress, tosendStatus);
				writeToNode(progress, tosendDelay);
			}

			Message m = fromNode.read();
			if (m.name == "Instructions") {
				request_t newReq;
				RF24NetworkHeader header(/*to node*/ amp_node);
				char instruction = m.data[0];
				switch (instruction) {
					case 'I':
						newReq = { POWER_UP };
						while (!(network.write(header, &newReq, sizeof(newReq)))) {
							printf("Failed to send\n");
						}
						printf("Successfully sent\n");
						break;
					case 'O':
						newReq = { POWER_DOWN };
						while (!(network.write(header, &newReq, sizeof(newReq)))) {
							printf("Failed to send\n");
						}
						printf("Successfully sent\n");
						break;
					case 'S':
						newReq = { UPDATE_STATE };
						while (!(network.write(header, &newReq, sizeof(newReq)))) {
							printf("Failed to send\n");
						}
						printf("Successfully sent\n");
						break;
					default:
						break;
				}
			}
		}
	}
};

StreamingWorker * create_worker(Callback *data
	, Callback *complete
	, Callback *error_callback, v8::Local<v8::Object> & options) {
	return new HomeKitAmp(data, complete, error_callback, options);
}

NODE_MODULE(HomeKitAmp, StreamWorkerWrapper::Init)
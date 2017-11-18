#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Bounce2.h>

#define RELAY_PIN 5
#define SWITCH_PIN 6
#define SAFETY_DELAY_MS 20000	  //20s of delay before powering amp again after power down

RF24 radio(9, 10);                // nRF24L01(+) radio attached on Arduino Pro Mini 

RF24Network network(radio);      // Network initialized with that radio

Bounce debouncer = Bounce(); 

const uint16_t amp_node = 01;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t rpi_node = 00;   // Address of the other node in Octal format

unsigned long lastPowerDownTime; //Time of the last power down
bool activeRelay;				 //Relay on or off
bool lastSwitchState;

typedef enum {NONE, POWER_UP, POWER_DOWN, UPDATE_STATE}Action;
Action pendingAction;

typedef struct {                 // Structure of our request
	Action actionReq;				 // 'I' => POWER_ON - 'O' => POWER_DOWN - 'S' => UPDATE_STATE
} request_t;

typedef struct {				   //Structure of our answer
	char currentState;			   // ON => 'I', OFF => 'O'
//	char actionStatusCode;				   //OK : 0 - Error : 1
	uint16_t delay;				   //delay in s
} answer_t;

uint16_t powerUpAmp(void) {	  //Check the delay : if OK power on and return 0 else return delay time in ms
	unsigned long currentTime = millis();

	if((unsigned long)(currentTime - lastPowerDownTime) >= SAFETY_DELAY_MS){
		digitalWrite(RELAY_PIN, HIGH);
		activeRelay = true;
		pendingAction = NONE;
		return 0;
	} else {
		return (uint16_t)(SAFETY_DELAY_MS - (currentTime - lastPowerDownTime));
	}
}

void powerDownAmp(void){	//Power down the amp and save the time for the safety delay
	lastPowerDownTime = millis();
	digitalWrite(RELAY_PIN, LOW);
	activeRelay = false;
	pendingAction = NONE;
}

void checkSwitchAction(void){
	bool switchState = debouncer.read();
	if(switchState != lastSwitchState){
		if(pendingAction == NONE){			//If there is already a pending action, cancel it
			pendingAction = !activeRelay ? POWER_UP : POWER_DOWN;
		} else {
			pendingAction = NONE;
		}
		lastSwitchState = switchState;
	}
}

void checkRadioAction(void){
	while (network.available()) {     // Check for incoming radio data
		RF24NetworkHeader header;
		request_t request;
		network.read(header, &request, sizeof(request));

		Serial.print("Received request with action :");
		Serial.println(request.actionReq);
		
		switch(request.actionReq){
			case POWER_UP:
				pendingAction = pendingAction != POWER_DOWN ? POWER_UP : NONE;		//If there is currently a power down action, cancel it
				break;
			case POWER_DOWN:
				pendingAction = pendingAction != POWER_UP ? POWER_DOWN : NONE;		//If there is currently a power up action, cancel it
				break;
			case UPDATE_STATE:
				pendingAction = UPDATE_STATE;
				break;
			default:
				pendingAction = NONE;
		}
	}
}

void performPendingAction(void){
	uint16_t delay=0;
	switch(pendingAction){
		case NONE:
			break;
		case UPDATE_STATE:
			sendNewState(delay);
			break;
		case POWER_UP:
			delay = powerUpAmp();
			sendNewState(delay);
			break;
		case POWER_DOWN:
			powerDownAmp();
			sendNewState(delay);
			break;
		default:
			break;
	}
}

void sendNewState(uint16_t delay){
	answer_t answer;
	answer.currentState = activeRelay == 1 ? 'I' : 'O';
	answer.delay = delay;
	RF24NetworkHeader header(rpi_node);
	Serial.print("sendNewStatus - currentState : ");
	Serial.print(answer.currentState);
	Serial.print(", delay : ");
	Serial.print(delay);
	Serial.println("ms");
	bool ok = network.write(header, &answer, sizeof(answer));
	if (ok) {
		Serial.println("sendNewStatus - success");
	}
	else {
		Serial.println("sendNewStatus - fail");
	}
}

void setup(void)
{
	pinMode(RELAY_PIN, OUTPUT);
	pinMode(SWITCH_PIN, INPUT);

	// After setting up the button, setup the Bounce instance :
	debouncer.attach(SWITCH_PIN);
	debouncer.interval(200); // interval in ms	
	debouncer.update();
	lastPowerDownTime = -20000; //for the startup
	activeRelay = false;
	lastSwitchState = debouncer.read();
	pendingAction = NONE;

	Serial.begin(57600);
	Serial.println("HomeKitAmp Arduino Setup()");

	SPI.begin();
	radio.begin();
	network.begin(/*channel*/ 90, /*node address*/ amp_node);
}

void loop(void) {
	network.update();               // Check the network regularly
	debouncer.update();				// Update the Bounce instance

	checkSwitchAction();
	checkRadioAction();
	performPendingAction();
}

/*
 Name:		AcquisitionSketch_v1.ino
 Created:	10/04/2018 17:45:34
 Author:	calta
*/
#define PIN_CLR 2
#define PIN_CLK 3

enum STATES
{
	IDLE,
	INPUT_CHECK,
	LIVE_FEED
};

STATES actualState = IDLE;
STATES previousState = IDLE;
// Magnetic tile reading
int pixelOrder[] = { 26, 27, 18, 19, 10, 11, 2, 3, 1, 0, 9, 8, 17, 16, 25, 24 };
int subtileOrder[] = { 0, 2, 1, 3 };
int subtileOffset[] = { 0, 4, 32, 36 };
uint16_t frame[64];

int numFrames = 0;
int curFrame = 0;

void clearCounter() {
	digitalWrite(PIN_CLR, 0);
	//delay(10);
	digitalWrite(PIN_CLR, 1);
	//delay(10);
}

void incrementCounter() {
	digitalWrite(PIN_CLK, 1);
	//delay(1);
	digitalWrite(PIN_CLK, 0);
	//delay(1);
}

int readMagel(int number_of_samples = 2)
{
	int average = 0;
	for (int i = 0; i < number_of_samples; i++)
	{
		average += analogRead(A0);
	}
		return average /number_of_samples;
}

void readFrame() {

	clearCounter();
	incrementCounter();

	for (int curSubtileIdx = 0; curSubtileIdx < 4; curSubtileIdx++) {
		for (int curIdx = 0; curIdx < 16; curIdx++) {
			// Read value
			int value = readMagel(1);

			//Serial.println(value);
			//delay(10);

			// Store value in correct frame location
			int frameOffset = pixelOrder[curIdx] + subtileOffset[subtileOrder[curSubtileIdx]];
			//terminal.println(frameOffset);
			//delay(25);
			frame[frameOffset] = value;

			// Increment to next pixel
			incrementCounter();
		}
	}
}

// Display current frame on serial console
void displayCurrentFrame() {
	// Display frame
	//  terminal.println ("\nCurrent Frame");
	int idx = 0;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Serial.print(frame[idx]);
			Serial.print(" ");
			idx += 1;
		}
		Serial.println("");
	}
	Serial.println("*");
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(PIN_CLR, OUTPUT);
	pinMode(PIN_CLK, OUTPUT);
	analogReadResolution(12);
	incrementCounter();
	clearCounter();
}

// the loop function runs over and over again until power down or reset
void loop() {
	
		switch (actualState)
		{
		case IDLE:
			actualState = INPUT_CHECK;
			previousState = IDLE;
			break;
		case INPUT_CHECK:
		{
			if (Serial.available())
			{
				byte incoming = Serial.read();
				if (incoming == 'l')
				{
					actualState = LIVE_FEED;
				}
				else
				{
					actualState = IDLE;
				}
				while (Serial.available())
				{
					Serial.read();
				}
				previousState = INPUT_CHECK;
			}
			else
				actualState = previousState;
			break;
		}
		case LIVE_FEED:
			readFrame();
			displayCurrentFrame();
			previousState = LIVE_FEED;
			actualState = INPUT_CHECK;
			break;
		default:
			break;
		}
		
}

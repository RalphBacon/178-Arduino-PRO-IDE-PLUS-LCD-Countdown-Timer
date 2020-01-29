#include "Arduino.h"
#include "sketch.h"
#include <EEPROM.h>
#include <wire.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG 0

// Used for generating interrupts using CLK signal
const int PinA = 3;

// Used for reading DT signal
const int PinB = 4;

// Used for the push button switch
const int PinSW = 5;

// What unit are we changing (if any)?
static bool isMinsChanging = false;
static bool isSecsChanging = false;

// Previous mins/secs read from EEPROM
uint8_t lastMins = 0;
uint8_t lastSecs = 0;

// Updated by the ISR (Interrupt Service Routine)
volatile int rotaryPosition = 0;

// Confirmation beeper
#define beepPin 11

// LCD init
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// Forward declaration(s)
void updateTime();
void beep();
void printBigNum(int number, int startCol, int startRow);
void printCountDown(uint8_t lastMins, uint8_t lastSecs);

// ------------------------------------------------------------------
// INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT
// ------------------------------------------------------------------
void isr() {
	static unsigned long lastInterruptTime = 0;
	unsigned long interruptTime = millis();

	// Debouncing and slowing down update frequency
	const uint8_t updateFrequency = 10;

	// If interrupts come faster than 5ms, assume it's a bounce and ignore
	// And only accept changes here if we are actively updating mins/secs
	if ((isMinsChanging | isSecsChanging)
		&& interruptTime - lastInterruptTime > updateFrequency) {
		if (digitalRead(PinB) == LOW)
		{
			rotaryPosition--; // Could be -5 or -10
		}
		else {
			rotaryPosition++; // Could be +5 or +10
		}
	}
	// Keep track of when we were here last (no more than every 5ms)
	lastInterruptTime = interruptTime;
}

// ------------------------------------------------------------------
// SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    SETUP
// ------------------------------------------------------------------
void setup() {
	// Just whilst we debug, view output on serial monitor
#if DEBUG
	Serial.begin(115200);
#endif

	// Beeper pin - double confirmation beep
	pinMode(beepPin, OUTPUT);
	beep();
	delay(100);
	beep();

	// LCD initialise
	lcd.init();
	lcd.backlight();
	lcd.setContrast(100);

	// Read the previously stored Mins/Secs from EEPROM
	int storedMins = EEPROM.read(0);
	int storedSecs = EEPROM.read(1);
	lastMins = storedMins > 99 ? 0 : storedMins;
	lastSecs = storedSecs > 60 ? 0 : storedSecs;

	// Rotary pulses are INPUTs (have pullup resistors on module)
	pinMode(PinA, INPUT);
	pinMode(PinB, INPUT);

	// Switch is floating so use the in-built PULLUP so we don't need a resistor
	pinMode(PinSW, INPUT_PULLUP);

	// Create custom LCD character map (8 characters only!)
	for (unsigned int cnt = 0; cnt < sizeof(custChar) / 8; cnt++) {
		lcd.createChar(cnt, custChar[cnt]);
	}

	// Attach the routine to service the interrupts
	attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);

	// Ready to go!
	lcd.setCursor(0, 0);
}

// check if someone is pressing the switch to change the time
int timeChange(int lastCount) {
	// Is someone pressing the rotary switch?
	if ((!digitalRead(PinSW))) {
		lcd.clear();
		lcd.home();
		// Wait until released
		while (!digitalRead(PinSW)) {
			delay(100);
		}
		// Are we changing Minutes or Seconds or Nothing?
		if (isSecsChanging) {
			isSecsChanging = false;
			// Write to EEPROM
			EEPROM.update(1, lastSecs);
			beep();
			lcd.setCursor(0, 0);
			lcd.print("Saved OK.       ");
			delay(1000);
			lcd.clear();
#if DEBUG
			Serial.println("All changes saved.");
#endif
		} else {
			if (isMinsChanging) {
				isMinsChanging = false;
				isSecsChanging = true;
				lastCount = lastSecs;
				rotaryPosition = lastSecs;
				EEPROM.update(0, lastMins);
				lcd.setCursor(0, 0);
				lcd.print("Change seconds: ");
				updateTime();
#if DEBUG
				Serial.println("Change Seconds: ");
#endif
			} else {
				isMinsChanging = true;
				lastCount = lastMins;
				rotaryPosition = lastMins;
				lcd.setCursor(0, 0);
				lcd.print("Change minutes: ");
				updateTime();
#if DEBUG
				Serial.println("Change Minutes: ");
#endif
			}
			// User feedback here (eg beep)
			beep();
			delay(750);
		}
	}
	return lastCount;
}

// Update EEPROM values
void changeEEPROMvalues(int lastCount)
		{
	// If the current rotary switch position has changed then update everything
	if ((rotaryPosition != lastCount) && (isMinsChanging | isSecsChanging)) {
		// Write out to serial monitor the value and direction
#if DEBUG
		//Serial.print(rotaryPosition > lastCount ? "Up  :" : "Down:");
		//Serial.println(rotaryPosition);
#endif
		// We're changing MINUTES here, pretty simple
		if (isMinsChanging) {
			if (rotaryPosition > 99) {
				rotaryPosition = 99;
			}
			if (rotaryPosition < 0) {
				rotaryPosition = 0;
			}
			lastMins = rotaryPosition;
			lastCount = lastMins;
		} else        // We're changing SECONDS here, allow for over/underflow
		{
			if (rotaryPosition > 59) {
				lastMins = lastMins < 99 ? lastMins + 1 : lastMins;
				rotaryPosition = 0;
			} else
			if (rotaryPosition < 0) {
				lastMins = lastMins > 0 ? lastMins - 1 : lastMins;
				rotaryPosition = 59;
			}

			lastSecs = rotaryPosition;
			lastCount = lastSecs;
		}
		// Update the display
		updateTime();
	}
}

// ------------------------------------------------------------------
// MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP
// ------------------------------------------------------------------
void loop() {
	static int lastCount;
	static unsigned long lastMillis = millis();

	// Is someone pressing the rotary switch?
	lastCount = timeChange(lastCount);

	// If the current rotary switch position has changed then update everything
	changeEEPROMvalues(lastCount);

	// Tick tock
	if (millis() - lastMillis >= 1000) {
		if (lastSecs == 0) {
			lastSecs = lastMins == 0 ? 0 : 59;
			lastMins = lastMins == 0 ? 0 : lastMins - 1;
		} else {
			lastSecs--;
		}
		lastMillis = millis();

		if (!(isMinsChanging | isSecsChanging)) {
			printCountDown(lastMins, lastSecs);
		}
	}
}

// Big character countdown
void printCountDown(uint8_t Mins, uint8_t Secs) {
	// First digit
	int firstDigit = (Mins / 10);
	printBigNum(firstDigit, 0, 0);

	// Second Digit
	int secondDigit = abs(Mins % 10);
	printBigNum(secondDigit, 4, 0);

	// Colon Separator
	lcd.setCursor(8, 0);
	lcd.write(0b10100001);
	lcd.setCursor(8, 1);
	lcd.write(0b10100001);

	int thirdDigit = (Secs / 10);
	printBigNum(thirdDigit, 10, 0);

	// Second Digit
	int fourthDigit = abs(Secs % 10);
	printBigNum(fourthDigit, 13, 0);
}

// Update the time values from 99:00 down to 00:00.
void updateTime() {
#if DEBUG
	Serial.print(lastMins < 10 ? "0" : "");
	Serial.print(lastMins);
	Serial.print(":");
	Serial.print(lastSecs < 10 ? "0" : "");
	Serial.println(lastSecs);
#endif
	lcd.setCursor(0, 1);
	lcd.print(lastMins < 10 ? "0" : "");
	lcd.print(lastMins);
	if (isMinsChanging) {
		lcd.cursor_on();
		lcd.setCursor(1, 1);
		return;
	}

	lcd.print(":");
	lcd.print(lastSecs < 10 ? "0" : "");
	lcd.print(lastSecs);
	if (isSecsChanging) {
		lcd.setCursor(4, 1);
		lcd.cursor_on();
		return;
	}
	lcd.cursor_off();
}

// Simple short beep
void beep() {
	digitalWrite(beepPin, HIGH);
	delay(100);
	digitalWrite(beepPin, LOW);
}

// -----------------------------------------------------------------
// Print big number over 2 lines, 3 columns per half digit
// -----------------------------------------------------------------
void printBigNum(int number, int startCol, int startRow) {

	// Position cursor to requested position (each char takes 3 cols plus a space col)
	lcd.setCursor(startCol, startRow);

	// Each number split over two lines, 3 chars per line. Retrieve character
	// from the main array to make working with it here a bit easier.
	uint8_t thisNumber[6];
	for (int cnt = 0; cnt < 6; cnt++) {
		thisNumber[cnt] = bigNums[number][cnt];
	}

	// First line (top half) of digit
	for (int cnt = 0; cnt < 3; cnt++) {
		lcd.print((char) thisNumber[cnt]);
	}

	// Now position cursor to next line at same start column for digit
	lcd.setCursor(startCol, startRow + 1);

	// 2nd line (bottom half)
	for (int cnt = 3; cnt < 6; cnt++) {
		lcd.print((char) thisNumber[cnt]);
	}
}

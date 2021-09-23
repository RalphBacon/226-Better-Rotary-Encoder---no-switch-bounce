#include <Arduino.h>

// Rotary encoder pins
#define PIN_A 32
#define PIN_B 4
#define PUSH_BTN 16

// A turn counter for the rotary encoder (negative = anti-clockwise)
int rotationCounter = 200;

// Flag from interrupt routine (moved=true)
volatile bool rotaryEncoder = false;

// Interrupt routine just sets a flag when rotation is detected
void IRAM_ATTR rotary()
{
    rotaryEncoder = true;
}

// Rotary encoder has moved (interrupt tells us) but what happened?
// See https://www.pinteric.com/rotary.html
int8_t checkRotaryEncoder()
{
    // Reset the flag that brought us here (from ISR)
    rotaryEncoder = false;

    static uint8_t lrmem = 3;
    static int lrsum = 0;
    static int8_t TRANS[] = {0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0};

    // Read BOTH pin states to deterimine validity of rotation (ie not just switch bounce)
    int8_t l = digitalRead(PIN_A);
    int8_t r = digitalRead(PIN_B);

    // Move previous value 2 bits to the left and add in our new values
    lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;

    // Convert the bit pattern to a movement indicator (14 = impossible, ie switch bounce)
    lrsum += TRANS[lrmem];

    /* encoder not in the neutral (detent) state */
    if (lrsum % 4 != 0)
    {
        return 0;
    }

    /* encoder in the neutral state - clockwise rotation*/
    if (lrsum == 4)
    {
        lrsum = 0;
        return 1;
    }

    /* encoder in the neutral state - anti-clockwise rotation*/
    if (lrsum == -4)
    {
        lrsum = 0;
        return -1;
    }

    // An impossible rotation has been detected - ignore the movement
    lrsum = 0;
    return 0;
}

void setup()
{
    Serial.begin(115200);

    // The module already has pullup resistors on board
    pinMode(PIN_A, INPUT);
    pinMode(PIN_B, INPUT);

    // But not for the push switch
    pinMode(PUSH_BTN, INPUT_PULLUP);

    // We need to monitor both pins, rising and falling for all states
    attachInterrupt(digitalPinToInterrupt(PIN_A), rotary, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_B), rotary, CHANGE);
    Serial.println("Setup completed");
}

void loop()
{
    // Has rotary encoder moved?
    if (rotaryEncoder)
    {
        // Get the movement (if valid)
        int8_t rotationValue = checkRotaryEncoder();

        // If valid movement, do something
        if (rotationValue != 0)
        {
            rotationCounter += rotationValue * 5;
            Serial.print(rotationValue < 1 ? "L" :  "R");
            Serial.println(rotationCounter);
        }
    }

    if (digitalRead(PUSH_BTN) == LOW)
    {
        rotationCounter = 0;
        Serial.print("X");
        Serial.println(rotationCounter);
 
        // Wait until button released (demo only! Blocking call!)
        while (digitalRead(PUSH_BTN) == LOW)
        {
            delay(100);
        }
    }
}
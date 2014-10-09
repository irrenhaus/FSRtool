#include <stdint.h>

#define INT16_MAX 0x7fff

#define DEBUG                     1

#define TRIGGER_PIN               12
#define LED_PIN                   13
#define TRIGGER_LEVEL             LOW

// Defines if readings should be averaged.
#define NUM_AVERAGE_READINGS      2

// How many readings should be averaged for getting the base-level value
#define BASE_LEVEL_AVG_READINGS   10

// Specify how long to wait for a trigger event before resetting
// the base-level value.
#define RESET_AFTER_SECONDS       5

// How many reads should occur per second
// This ignores the time needed for processing and the time needed for the
// analog reading itself.
// Max: 1000
// If set to 0 this value is ignored and the reading occurs as fast as possible.
#define READS_PER_SECOND          0

// As soon as the difference of the current reading to the previous/average
// reading is at least as high as this number, trigger the endstop
#define TRIGGER_HIGH_MARK         32
// As soon as the negative difference of the current reading to the
// previous/average reading is at least as high as this number, reset
// the endstop
#define TRIGGER_LOW_MARK          16

// The analog pins on which the FSRs can be read
uint8_t analogIn[3] = {A0, A1, A2};

uint8_t  triggered[3] = {0, 0, 0};
uint32_t lastBaseLevelReset = 0;
uint16_t baseLevel[3] = {0, 0, 0};
uint16_t reading[3] = {0, 0, 0};

void readBaseLevel() {
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t fsr = 0;
        for(uint8_t r = 0; r < BASE_LEVEL_AVG_READINGS; r++) {
            fsr += analogRead(analogIn[i]);
        }
        fsr /= BASE_LEVEL_AVG_READINGS;
        baseLevel[i] = fsr;
    }
}

void setup()
{
#if DEBUG > 0
    Serial.begin(115200);
#endif

    // Enable 5V output so that we can read the FSRs...
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);

    // Setup the analog inputs
    analogReference(DEFAULT);

    for(uint8_t i = 0; i < 3; i++) {
        pinMode(analogIn[i], INPUT);
        digitalWrite(analogIn[i], LOW);
    }

    readBaseLevel();
}

void loop()
{
    for(uint8_t i = 0; i < 3; i++) {
#if NUM_AVERAGE_READINGS > 0
        reading[i] = 0;
        for(uint8_t r = 0; r < NUM_AVERAGE_READINGS; r++) {
            reading[i] += analogRead(analogIn[i]);
        }
        reading[i] /= NUM_AVERAGE_READINGS;
#else
        reading[i] = analogRead(analogIn[i]);
#endif

        int16_t lowMark = max(0, int16_t(baseLevel[i] - TRIGGER_LOW_MARK));
        int16_t highMark = min(INT16_MAX, int16_t(baseLevel[i] + TRIGGER_HIGH_MARK));

        if(reading[i] > highMark) {
            triggered[i] = 1;
            // Reset the base-level-reset timer
            lastBaseLevelReset = millis();
        } else if(reading[i] < lowMark) {
            triggered[i] = 0;
            // Reset the base-level-reset timer
            lastBaseLevelReset = millis();
        } else {
            if((millis() - lastBaseLevelReset) > (RESET_AFTER_SECONDS * 1000)) {
                readBaseLevel();
                lastBaseLevelReset = millis();
            }
        }
    }

#if DEBUG > 0
    Serial.print("Triggered: a:");
    Serial.print(triggered[0]);
    Serial.print(" b:");
    Serial.print(triggered[1]);
    Serial.print(" c:");
    Serial.println(triggered[2]);
#endif

    if(triggered[0] + triggered[1] + triggered[2] > 0) {
        digitalWrite(TRIGGER_PIN, TRIGGER_LEVEL);
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(TRIGGER_PIN, !TRIGGER_LEVEL);
        digitalWrite(LED_PIN, LOW);
    }

#if READS_PER_SECOND > 0
    delay(1000 / READS_PER_SECOND);
#endif
}

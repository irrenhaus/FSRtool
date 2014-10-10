#include <stdint.h>

#define DEBUG                     1

#define TRIGGER_PIN               12
#define LED_PIN                   13
#define TRIGGER_LEVEL             LOW

#define MAX_TRIGGERS_PER_SECOND   4

// Defines if readings should be averaged.
#define NUM_AVERAGE_READINGS      0

// How many readings should be averaged for getting the base-level value
#define BASE_LEVEL_AVG_READINGS   10

// Specify how long to wait for a trigger event before resetting
// the base-level value.
#define RESET_AFTER_SECONDS       5

#define FLAP_DETECTION_BUFFER     64

// How many reads should occur per second
// This ignores the time needed for processing and the time needed for the
// analog reading itself.
// Max: 1000
// If set to 0 this value is ignored and the reading occurs as fast as possible.
#define READS_PER_SECOND          0

// As soon as the difference of the current reading to the previous/average
// reading is at least as high as this number, trigger the endstop
#define TRIGGER_HIGH_MARK         128
// As soon as the negative difference of the current reading to the
// previous/average reading is at least as high as this number, reset
// the endstop
#define TRIGGER_LOW_MARK          16

#define MIN_TIME_PER_TRIGGER      (1000 / MAX_TRIGGERS_PER_SECOND)

// The analog pins on which the FSRs can be read
uint8_t analogIn[3] = {A0, A1, A2};

uint8_t  triggered[3] = {0, 0, 0};
unsigned long lastBaseLevelReset = 0;
uint16_t baseLevel[3] = {0, 0, 0};
uint16_t reading[3] = {0, 0, 0};
uint16_t highMark[3] = {0, 0, 0};
uint16_t lowMark[3] = {0, 0, 0};

unsigned long lastTriggerTime[3] = {0, 0, 0};
unsigned long lastDebugOutput = 0;
uint8_t  baseLevelReadPending[3] = {0, 0, 0};

void readBaseLevelForFSR(uint8_t in) {
    uint32_t fsr = 0;
    for(uint8_t r = 0; r < BASE_LEVEL_AVG_READINGS; r++) {
        fsr += analogRead(analogIn[in]);
    }
    fsr /= BASE_LEVEL_AVG_READINGS;
    baseLevel[in] = fsr;

    triggered[in] = 0;

    highMark[in] = min(1023, int16_t(baseLevel[in] + TRIGGER_HIGH_MARK));
}

void readBaseLevel() {
    for(uint8_t i = 0; i < 3; i++) {
        readBaseLevelForFSR(i);
    }
}

void setup()
{
#if DEBUG > 0
    Serial.begin(115200);
#endif

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(TRIGGER_PIN, OUTPUT);
    digitalWrite(TRIGGER_PIN, ! TRIGGER_LEVEL);

    // Setup the analog inputs
    analogReference(DEFAULT);

    for(uint8_t i = 0; i < 3; i++) {
        pinMode(analogIn[i], INPUT);
        digitalWrite(analogIn[i], LOW);
    }

    readBaseLevel();
    lastBaseLevelReset = millis();
}

void loop()
{
    uint8_t triggerChanged = 0;

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

        if(reading[i] > highMark[i]) {
            if(!triggered[i] && (millis() - lastTriggerTime[i]) >= MIN_TIME_PER_TRIGGER) {
                lastTriggerTime[i] = millis();
                lastBaseLevelReset = millis();
                triggerChanged = 1;
                triggered[i] = 1;
            }

            if(lowMark[i] <= TRIGGER_LOW_MARK || reading[i] > (lowMark[i] - TRIGGER_LOW_MARK)) {
                // If we're reading higher values than before update the trigger low mark
                lowMark[i] = max(0, int16_t(reading[i] - TRIGGER_LOW_MARK));
            }
        } else if((reading[i] < lowMark[i] || baseLevel[i] < TRIGGER_LOW_MARK)) {
            if(triggered[i]) {
                lastTriggerTime[i] = millis();
                triggerChanged = 1;
                triggered[i] = 0;
            }
        }
    }

    if((millis() - lastBaseLevelReset) > (unsigned long)(RESET_AFTER_SECONDS * 1000)) {
        Serial.println("Reset base level!");
        readBaseLevel();
        lastBaseLevelReset = millis();

        triggerChanged = 1;
    }

    if(triggered[0] + triggered[1] + triggered[2] > 0) {
        digitalWrite(TRIGGER_PIN, TRIGGER_LEVEL);
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(TRIGGER_PIN, !TRIGGER_LEVEL);
        digitalWrite(LED_PIN, LOW);
    }

#if DEBUG > 0
    if(millis() - lastDebugOutput > 500 || triggerChanged) {
        lastDebugOutput = millis();

        if(triggerChanged) {
            Serial.print("Low mark: a:");
            Serial.print(lowMark[0]);
            Serial.print(" b:");
            Serial.print(lowMark[1]);
            Serial.print(" c:");
            Serial.println(lowMark[2]);
        }

        Serial.print("Triggered: a:");
        Serial.print(triggered[0]);
        Serial.print(" b:");
        Serial.print(triggered[1]);
        Serial.print(" c:");
        Serial.println(triggered[2]);

        Serial.print("Reading: a:");
        Serial.print(reading[0]);
        Serial.print(" b:");
        Serial.print(reading[1]);
        Serial.print(" c:");
        Serial.println(reading[2]);

        Serial.print("BaseLevel: a:");
        Serial.print(baseLevel[0]);
        Serial.print(" b:");
        Serial.print(baseLevel[1]);
        Serial.print(" c:");
        Serial.println(baseLevel[2]);

        Serial.print("Last reset:");
        Serial.print(millis() - lastBaseLevelReset);
        Serial.println("ms ago");

        Serial.println(" ");
    }
#endif


#if READS_PER_SECOND > 0
    delay(1000 / READS_PER_SECOND);
#endif
}

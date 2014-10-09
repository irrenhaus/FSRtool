#include <stdint.h>

#define INT16_MAX 0x7fff

#define DEBUG                     1

#define TRIGGER_PIN               12
#define LED_PIN                   13
#define TRIGGER_LEVEL             LOW

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
#define TRIGGER_HIGH_MARK         64
// As soon as the negative difference of the current reading to the
// previous/average reading is at least as high as this number, reset
// the endstop
#define TRIGGER_LOW_MARK          16

// The analog pins on which the FSRs can be read
uint8_t analogIn[3] = {A0, A1, A2};

uint8_t  triggered[4] = {0, 0, 0, 0};
unsigned long lastBaseLevelReset = 0;
uint16_t baseLevel[3] = {0, 0, 0};
uint16_t reading[3] = {0, 0, 0};

uint8_t flapDetectionBuf[FLAP_DETECTION_BUFFER];
uint8_t  flapPos = 0;
uint8_t  flapDataPoints = 0;

uint8_t  detectFlapping(uint8_t changed) {
    flapDetectionBuf[flapPos] = changed;
    if(flapPos >= FLAP_DETECTION_BUFFER) {
        flapPos = 0;
    } else {
        flapPos++;
    }

    if(flapDataPoints < FLAP_DETECTION_BUFFER) {
        flapDataPoints++;
    }

    uint8_t changes = 0;
    for(uint8_t i = 0; i < flapDataPoints; i++) {
        changes += flapDataPoints;
    }

    return (changes > (flapDataPoints/2));
}

void readBaseLevel() {
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t fsr = 0;
        for(uint8_t r = 0; r < BASE_LEVEL_AVG_READINGS; r++) {
            fsr += analogRead(analogIn[i]);
        }
        fsr /= BASE_LEVEL_AVG_READINGS;
        baseLevel[i] = fsr;

        triggered[i] = 0;
    }
    triggered[3] = 0;
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

        int16_t lowMark = max(0, int16_t(baseLevel[i] - TRIGGER_LOW_MARK));
        int16_t highMark = min(INT16_MAX, int16_t(baseLevel[i] + TRIGGER_HIGH_MARK));

        if(reading[i] > highMark) {
            if(!triggered[i]) {
                lastBaseLevelReset = millis();
                triggerChanged = 1;
                triggered[i] = 1;
            }
        } else if(triggered[i] && (reading[i] < lowMark || baseLevel[i] < TRIGGER_LOW_MARK)) {
            triggerChanged = 1;
            triggered[i] = 0;
        }

    }

    int16_t accuRead = reading[0] + reading[1] + reading[2];
    int16_t accuBase = baseLevel[0] + baseLevel[1] + baseLevel[2];

    int16_t accuLowMark = max(0, int16_t(accuBase - TRIGGER_LOW_MARK*2));
    int16_t accuHighMark = min(INT16_MAX, int16_t(accuBase + TRIGGER_HIGH_MARK*2));

    if(accuRead > accuHighMark) {
        if(!triggered[3]) {
            lastBaseLevelReset = millis();
            triggerChanged = 1;
            triggered[3] = 1;
        }
    } else if(triggered[3] && (accuRead < accuLowMark || accuBase < TRIGGER_LOW_MARK)) {
        triggerChanged = 1;
        triggered[3] = 0;
    }

    if((millis() - lastBaseLevelReset) > (unsigned long)(RESET_AFTER_SECONDS * 1000)) {
        Serial.println("Reset base level!");
        readBaseLevel();
        lastBaseLevelReset = millis();

        triggerChanged = 1;
    }

    if(detectFlapping(triggerChanged)) {
        // We are flapping, for safety assume triggered.
        triggered[3] = 1;

        Serial.println("FLAPPING");
    }

    if(triggered[0] + triggered[1] + triggered[2] + triggered[3] > 0) {
        digitalWrite(TRIGGER_PIN, TRIGGER_LEVEL);
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(TRIGGER_PIN, !TRIGGER_LEVEL);
        digitalWrite(LED_PIN, LOW);
    }

#if DEBUG > 0
    if(triggerChanged) {
        Serial.print("Triggered: a:");
        Serial.print(triggered[0]);
        Serial.print(" b:");
        Serial.print(triggered[1]);
        Serial.print(" c:");
        Serial.print(triggered[2]);
        Serial.print(" d:");
        Serial.println(triggered[3]);

        Serial.print("Reading: a:");
        Serial.print(reading[0]);
        Serial.print(" b:");
        Serial.print(reading[1]);
        Serial.print(" c:");
        Serial.print(reading[2]);
        Serial.print(" d:");
        Serial.println(accuRead);

        Serial.print("BaseLevel: a:");
        Serial.print(baseLevel[0]);
        Serial.print(" b:");
        Serial.print(baseLevel[1]);
        Serial.print(" c:");
        Serial.print(baseLevel[2]);
        Serial.print(" d:");
        Serial.println(accuBase);

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

#define EI_ARDUINO_INTERRUPTED_PIN
#include <Arduino.h>
#include <EnableInterrupt.h>
#include <TimerOne.h>
#include <avr/sleep.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define INITIAL_TIMER 100000
#define TIME_FACTOR 100
#define GAME_OVER_TIME 1000
#define FADE 5
#define DIM 4
#define START_PIN 10
#define START_BUTTON 4
#define DISTANCE (START_PIN - START_BUTTON)
#define ELAPSED 100
#define EASY_DECREASING 10
#define MEDIUM_DECREASING 15
#define HARD_DECREASING 20
#define EXTREME_DECREASING 30
// pin 3 because it's not used by timer 1
#define LED_S 3
#define POT A0
#define QUARTER 255
#define MAX_BRIGHTNESS 255
#define MAX_NUMBER 15
#define DECREASING_FACTOR 20
#define BUTTON_1 7
#define BUTTON_2 6
#define BUTTON_3 5
#define BUTTON_4 4

enum Difficulty {
    EASY,
    MEDIUM,
    HARD,
    EXTREME,
};

enum State {
    INITIAL,
    SETTING_DIFFICULTY,
    SLEEP,
    START,
    GAME_OVER,
};

void sleep();
void interruptButton();
void initialPhase();
void gamePhase();
void sleepPhase();
void gameOverPhase();
void fading();
void startGamePhase();
void setUpGamePhase();
void checkResult();
void turnOffLeds();
void restartGame();
void settingDifficultyPhase();
void defaultDifficulty();
Difficulty mapDifficulty(int value);



// GENERAL
volatile int pins[DIM];
volatile int pinsState[DIM];
int buttons[DIM];

// PHASE 0
boolean alreadySetUpInitialPhase;
int fade;
volatile enum State state;
volatile int counterTimer;
boolean isFirstGame;

// PHASE 1
int number;
int score;
uint32_t gameTime;
volatile boolean alreadySetUpGamePhase;
boolean enterGamePhase;
int decreasingFactor;

// PHASE SETTING_DIFFICULTY
boolean alreadySetUpDifficultyPhase;

// PHASE 2
boolean alreadySetUpGameOverPhase;

volatile boolean isAwake;

int prev, brightness, currentDifficulty;
// prev is unsigned long int due to overflow when the arduino is on for a long time
volatile unsigned long int prevTime;
volatile enum Difficulty gameDifficulty;
volatile int count;

void setup() {
    // PHASE 0: initialize game state
    state = INITIAL;
    fade = FADE;
    isFirstGame = true;
    alreadySetUpInitialPhase = false;
    counterTimer = 0;
    Timer1.initialize(INITIAL_TIMER);
    Timer1.attachInterrupt(sleep);

    // PHASE 1: game phase
    srand(time(NULL));
    score = 0;
    enterGamePhase = false;
    alreadySetUpGamePhase = false;
    gameTime = INITIAL_TIMER;
    prev = 0;
    brightness = 0;
    prevTime = 0;
    // PHASE GAME OVER
    alreadySetUpGameOverPhase = false;
    isAwake = false;
    gameDifficulty = EASY;
    // setup pins (output)
    for (int i = 0; i < DIM; i++) {
        pins[i] = i + START_PIN;
        pinsState[i] = 0;
        pinMode(pins[i], OUTPUT);
    }
    pinMode(LED_S, OUTPUT);
    // setup buttons (input)
    for (int i = 0; i < DIM; i++) {
        buttons[i] = i + START_BUTTON;
        pinMode(buttons[i], INPUT);
    }

    // setup interrupts
    enableInterrupt(buttons[0], interruptButton, RISING);
    enableInterrupt(buttons[1], interruptButton, RISING);
    enableInterrupt(buttons[2], interruptButton, RISING);
    enableInterrupt(buttons[3], interruptButton, RISING);

    // count variable for debugging
    count = 0;

    Serial.begin(9600);
}

void loop() {
    // debugButtons();
    switch (state) {
        case INITIAL:
            initialPhase();
            break;
        case SETTING_DIFFICULTY:
            settingDifficultyPhase();
            break;
        case START:
            gamePhase();
            break;
        case SLEEP:
            sleepPhase();
            break;
        case GAME_OVER:
            gameOverPhase();
            break;
    }
}

void initialPhase() {
    if (!isFirstGame && !alreadySetUpInitialPhase) {
        Timer1.detachInterrupt();
        Timer1.setPeriod(INITIAL_TIMER);
        Timer1.restart();
        Timer1.attachInterrupt(sleep);
        score = 0;
        brightness = 0;
        counterTimer = 0;
        fade = FADE;
        gameTime = INITIAL_TIMER;
        alreadySetUpDifficultyPhase = false;
        alreadySetUpGamePhase = false;
        enterGamePhase = false;
        alreadySetUpGameOverPhase = false;
    }
    if (!alreadySetUpInitialPhase) {
        alreadySetUpInitialPhase = true;
        isFirstGame = false;
        Serial.println("Welcome to GMN\nPress B1 to start!");
    }
    fading();
    // POLLING
    noInterrupts();
    if (digitalRead(buttons[DIM - 1]) == HIGH) {
        delay(ELAPSED);
        state = SETTING_DIFFICULTY;
    }
    interrupts();
}

void fading() {
    analogWrite(LED_S, brightness);
    brightness += fade;
    if (brightness <= 0 || brightness >= MAX_BRIGHTNESS) {
        fade = -fade;
    }
    delay(20);
}

void settingDifficultyPhase() {
    if (!alreadySetUpDifficultyPhase) {
        Serial.println("Set difficulty of the game...");
        alreadySetUpDifficultyPhase = true;
        counterTimer = 0;
        Timer1.detachInterrupt();
        Timer1.setPeriod(2 * INITIAL_TIMER);
        Timer1.restart();
        Timer1.attachInterrupt(defaultDifficulty);
    }
    int newValue = analogRead(POT);
    Difficulty newGameDifficulty = mapDifficulty(newValue);
    if (gameDifficulty != newGameDifficulty) {
        gameDifficulty = newGameDifficulty;
        Serial.println(gameDifficulty);
        Serial.println("STATE: " + String(state));
    }
}

Difficulty mapDifficulty(int value) {
  if (value < QUARTER){
    return EASY;
  }else if( value >= QUARTER && value < 2 * QUARTER ){
    return MEDIUM;
  }else if( value >= 2 * QUARTER && value < 3 * QUARTER){
    return HARD;
  }else{
    return EXTREME;
  }
}

void defaultDifficulty() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        Serial.println("TEMPO SCADUTO DECIDO IO!");
        state = START;
    }
}

void gamePhase() {
    if (!enterGamePhase) {
        enterGamePhase = true;
        startGamePhase();
    }
    if (!alreadySetUpGamePhase) {
        alreadySetUpGamePhase = true;
        setUpGamePhase();
    }
}
void startGamePhase() {
    Serial.println("GO!");
    digitalWrite(LED_S, LOW);
    counterTimer = 0;
    Timer1.detachInterrupt();
    Timer1.setPeriod(gameTime);
    Timer1.restart();
    Timer1.attachInterrupt(checkResult);
    switch (gameDifficulty) {
        case EASY:
            decreasingFactor = EASY_DECREASING;
            break;
        case MEDIUM:
            decreasingFactor = MEDIUM_DECREASING;
            break;
        case HARD:
            decreasingFactor = HARD_DECREASING;
            break;
        case EXTREME:
            decreasingFactor = EXTREME_DECREASING;
            break;
    }
}

void setUpGamePhase() {
    turnOffLeds();
    number = rand() % (MAX_NUMBER + 1);
    Serial.println("Write in binary: " + String(number));
}

// INTERRUPT
void checkResult() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        Serial.println("Time to check: " + String(millis()));
        int current = 0;
        for (int i = 0; i < DIM; i++) {
            if (pinsState[i] == HIGH) {
                current += 1 << i;
            }
        }
        if (current == number) {
            score++;
            Serial.println("Decreasing Factor: " + String(decreasingFactor) + " for a difficulty of " + String(gameDifficulty));
            Serial.println("GOOD! SCORE: " + String(score));
            Serial.println("Game time before: " + String(gameTime));
            gameTime = (gameTime) * (100 - decreasingFactor) / 100;
            Serial.println("Game time after: " + String(gameTime));
            alreadySetUpGamePhase = false;
            counterTimer = 0;
            Timer1.setPeriod(gameTime);
            Timer1.restart();
        } else {
            state = GAME_OVER;
        }
    }
}

void gameOverPhase() {
    if (!alreadySetUpGameOverPhase) {
        alreadySetUpGameOverPhase = true;
        counterTimer = 0;
        Serial.println("GAME OVER! - Final Score: " + String(score));
        Timer1.detachInterrupt();
        Timer1.setPeriod(INITIAL_TIMER);
        Timer1.restart();
        Timer1.attachInterrupt(restartGame);
        turnOffLeds();
        digitalWrite(LED_S, HIGH);
        delay(GAME_OVER_TIME);
        digitalWrite(LED_S, LOW);
    }
}

// RESTART
void restartGame() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        state = INITIAL;
        alreadySetUpInitialPhase = false;
    }
}

void turnOffLeds() {
    for (int i = 0; i < DIM; i++) {
        digitalWrite(pins[i], LOW);
        pinsState[i] = LOW;
    }
}

void sleepPhase() {
    Serial.println("GOING IN POWER MODE...");
    Serial.flush();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
    sleep_disable();
    if (isAwake) {
        isAwake = false;
        Serial.println("WAKING UP.....");
        state = INITIAL;
        alreadySetUpInitialPhase = false;
    }
    delay(2 * ELAPSED);
}

void sleep() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        state = SLEEP;
        digitalWrite(LED_S, LOW);
        Serial.println(("SLEEPING!"));
        Serial.println(millis());
        counterTimer = 0;
    }
}

void interruptButton() {
    switch (state) {
        case START: {
            unsigned long int time = millis();
            if (time - prevTime > ELAPSED) {
                prevTime = time;
                count++;
                switch (arduinoInterruptedPin) {
                    case BUTTON_4:
                        pinsState[0] = !pinsState[0];
                        digitalWrite(pins[0], pinsState[0]);
                        break;
                    case BUTTON_3:
                        pinsState[1] = !pinsState[1];
                        digitalWrite(pins[1], pinsState[1]);
                        break;
                    case BUTTON_2:
                        pinsState[2] = !pinsState[2];
                        digitalWrite(pins[2], pinsState[2]);
                        break;
                    case BUTTON_1:
                        pinsState[3] = !pinsState[3];
                        digitalWrite(pins[3], pinsState[3]);
                        break;
                }
            }
            break;
        }
        case SETTING_DIFFICULTY:
            Serial.println("DIFFICULTY SELECTED");
            Serial.println("DIFFICULTY: " + String(gameDifficulty));
            state = START;
            break;
        case SLEEP:
            isAwake = true;
            break;
    }
}

void debugButtons() {
    noInterrupts();
    int current = count;
    interrupts();
    if (current != prev) {
        Serial.println(current);
        prev = current;
    }
}

#define EI_ARDUINO_INTERRUPTED_PIN
#include <EnableInterrupt.h>
#include <TimerOne.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <time.h>
#include <Arduino.h>

#include "General.h"
#include "LCD.h"

// INITIAL PHASE
volatile boolean alreadySetUpInitialPhase;
int brightness;
int fade;
volatile enum State state;
volatile int counterTimer;
boolean isFirstGame;

//SETTING DIFFICULTY PHASE
boolean alreadySetUpDifficultyPhase;
volatile enum Difficulty gameDifficulty;
int currentDifficulty;

//GAME PHASE
volatile boolean alreadySetUpGamePhase;
int number;
boolean enterGamePhase;
int decreasingFactor;
int score;
long int gameTime;
volatile boolean isCorrect;
// prev is unsigned long int due to overflow when the arduino is on for a long time
volatile unsigned long int prevTime;

//GAME OVER PHASE
boolean alreadySetUpGameOverPhase;

//SLEEPING PHASE
volatile boolean isSleeping;
volatile boolean isAwake;

//LEDS & BUTTONS
volatile int pins[DIM];
volatile int pinsState[DIM];
int buttons[DIM];


void setup() {
    // INITIALIZE PHASE
    state = INITIAL;
    fade = FADE;
    isFirstGame = true;
    alreadySetUpInitialPhase = false;
    brightness = 0;
    counterTimer = 0;
    Timer1.initialize(INITIAL_TIMER);
    Timer1.attachInterrupt(sleep);
    lcdInit();

    // GAME PHASE
    srand(time(NULL));
    score = 0;
    enterGamePhase = false;
    alreadySetUpGamePhase = false;
    gameTime = INITIAL_TIMER;
    prevTime = 0;
    isCorrect = false;

    // GAME OVER PHASE
    alreadySetUpGameOverPhase = false;
    isAwake = false;
    isSleeping = false;
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

    Serial.begin(9600);
}

void loop() {
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

//INITIAL PHASE
void initialPhase() {
    if (!isFirstGame && !alreadySetUpInitialPhase) {
        setUpTimer(sleep, INITIAL_TIMER);
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
        printInitPhase();
    }
    fading();
    // POLLING
    if (digitalRead(buttons[DIM - 1]) == HIGH) {
        delay(ELAPSED);
        state = SETTING_DIFFICULTY;
    }
}

void fading() {
    analogWrite(LED_S, brightness);
    brightness += fade;
    if (brightness <= 0 || brightness >= MAX_BRIGHTNESS) {
        fade = -fade;
    }
    delay(20);
}

//SETTING DIFFICULTY PHASE
void settingDifficultyPhase() {
    if (!alreadySetUpDifficultyPhase) {
        printDifficultyPhaseInit();
        alreadySetUpDifficultyPhase = true;
        counterTimer = 0;
        setUpTimer(defaultDifficulty, 2*INITIAL_TIMER);
        digitalWrite(LED_S, LOW);
    }
    int newValue = analogRead(POT);
    Difficulty newGameDifficulty = mapDifficulty(newValue);
    if (gameDifficulty != newGameDifficulty) {
        gameDifficulty = newGameDifficulty;
        printUpdateDifficulty();

    }
}

void defaultDifficulty() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        state = START;
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

//GAME PHASE
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
    Timer1.stop();
    printStartGamePhase();
    delay(ELAPSED * 10);
    counterTimer = 0;
    setUpTimer(checkResult, gameTime);
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
    if(isCorrect){
      score++;
      Timer1.stop();
      printGameSetUp();
      delay(ELAPSED * 20);
      gameTime = (gameTime) * (100 - decreasingFactor) / 100;
      counterTimer = 0;
      Timer1.setPeriod(gameTime);
      Timer1.restart();
      isCorrect = false;
    }
    turnOffLeds();
    number = rand() % (MAX_NUMBER + 1);
    printNumber();
}

void checkResult() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        int current = 0;
        for (int i = 0; i < DIM; i++) {
            if (pinsState[i] == HIGH) {
                current += 1 << i;
            }
        }
        if (current == number) {
            isCorrect = true;
            alreadySetUpGamePhase = false;
        } else {
            state = GAME_OVER;
        }
    }
}

//GAME OVER PHASE
void gameOverPhase() {
    if (!alreadySetUpGameOverPhase) {
        alreadySetUpGameOverPhase = true;
        counterTimer = 0;
        printGameOver();
        setUpTimer(restartGame, INITIAL_TIMER);
        turnOffLeds();
        digitalWrite(LED_S, HIGH);
        delay(GAME_OVER_TIME);
        digitalWrite(LED_S, LOW);
    }
}

void restartGame() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        state = INITIAL;
        alreadySetUpInitialPhase = false;
    }
}

//SLEEPING PHASE
void sleepPhase() {
    if (isSleeping){
      isSleeping = false;
      printSleeping();
    }
    Serial.flush();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
    sleep_disable();
    if (isAwake) {
        isAwake = false;
        state = INITIAL;
        alreadySetUpInitialPhase = false;
    }
    delay(2 * ELAPSED);
}

void sleep() {
    counterTimer++;
    if (counterTimer == TIME_FACTOR) {
        state = SLEEP;
        isSleeping = true;
        digitalWrite(LED_S, LOW);
        counterTimer = 0;
    }
}

//HELPER FUNCTIONS
void setUpTimer(void (*function)(), long int time) {
    Timer1.detachInterrupt();
    Timer1.setPeriod(time);
    Timer1.restart();
    Timer1.attachInterrupt(function);
}

void turnOffLeds() {
    for (int i = 0; i < DIM; i++) {
        digitalWrite(pins[i], LOW);
        pinsState[i] = LOW;
    }
}

//BUTTON HANDLER (INTERRUPT)
void interruptButton() {
    switch (state) {
        case START: {
            unsigned long int time = millis();
            if (time - prevTime > ELAPSED) {
                prevTime = time;
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
            delayMicroseconds(10000);
            state = START;
            break;
        case SLEEP:
            isAwake = true;
            break;
    }
}


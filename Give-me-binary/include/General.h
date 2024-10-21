#ifndef GENERAL_H
#define GENERAL_H

#define COL_POS(i) (3 + i + i * 3)
#define INITIAL_TIMER 100000
#define DIM 4
#define ELAPSED 100
#define TIME_FACTOR 100
#define GAME_OVER_TIME 1000
#define EASY_DECREASING 10
#define MEDIUM_DECREASING 15
#define HARD_DECREASING 20
#define EXTREME_DECREASING 30
#define MAX_NUMBER 15
#define FADE 5
#define MAX_BRIGHTNESS 255
#define POT A0
#define QUARTER 255
#define START_PIN 10
#define START_BUTTON 4
#define DECREASING_FACTOR 20
#define BUTTON_1 7
#define BUTTON_2 6
#define BUTTON_3 5
#define BUTTON_4 4
// pin 3 because it's not used by timer 1 (PWM works correctly)
#define LED_S 3

enum State {
    INITIAL,
    SETTING_DIFFICULTY,
    SLEEP,
    START,
    GAME_OVER,
};

enum Difficulty {
    EASY,
    MEDIUM,
    HARD,
    EXTREME,
};

extern int score;
extern volatile enum Difficulty gameDifficulty;
extern int number;

void settingDifficultyPhase();
void defaultDifficulty();
Difficulty mapDifficulty(int value);
void setUpTimer(void (*function)(), long int time);
void turnOffLeds();
void gameOverPhase();
void restartGame();
void gamePhase();
void startGamePhase();
void setUpGamePhase();
void checkResult();
void sleepPhase();
void sleep();
void initialPhase();
void fading();
void interruptButton();
void turnOffLeds();

#endif
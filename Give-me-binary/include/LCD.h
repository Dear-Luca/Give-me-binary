#ifndef LCD_H
#define LCD_H

#include <LiquidCrystal_I2C.h>

void lcdInit();
void printGameOver();
void printStartGamePhase();
void printGameSetUp();
void printNumber();
void printInitPhase();
void printDifficultyPhaseInit();
void printUpdateDifficulty();
void printSleeping();

#endif
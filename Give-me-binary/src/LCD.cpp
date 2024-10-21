#include "LCD.h"
#include "General.h"


static LiquidCrystal_I2C lcd(0x27, 20, 4);

void lcdInit(){
    lcd.init();
    lcd.backlight();
}

void printGameOver(){
    lcd.clear();
    lcd.setCursor(5,0);
    lcd.print("GAME OVER!");
    lcd.setCursor(9, 1);
    lcd.print("-");
    lcd.setCursor(2, 2);
    lcd.print("Final Score: " + String(score));
}

void printStartGamePhase(){
    lcd.clear();
    lcd.setCursor(8,1);
    lcd.print("GO!");
}

void printGameSetUp(){
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("GOOD! SCORE: " + String(score));
}

void printNumber(){
    lcd.clear();
    lcd.setCursor(9, 1);
    lcd.print(number);
}

void printInitPhase(){
    lcd.clear();
    lcd.setCursor(3,1);
    lcd.print("Welcome to GMB");
    lcd.setCursor(6,2);
    lcd.print("Press B1");
}

void printDifficultyPhaseInit(){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Set difficulty");
    lcd.setCursor(2,1);
    lcd.print("of the game...");
    for (int i = 0; i < DIM; i++){
          lcd.setCursor(COL_POS(i), 2);
          lcd.print(i+1);
          lcd.setCursor(COL_POS(i), 3);
          if (i == gameDifficulty){
            lcd.print("-");
          }else{
            lcd.print(" ");
          }
    }
}

void printUpdateDifficulty(){
    for (int i = 0; i < DIM; i++){
          lcd.setCursor(COL_POS(i), 3);
          if (i == gameDifficulty){
            lcd.print("-");
          }else{
            lcd.print(" ");
          }
    }
}

void printSleeping(){
    lcd.clear();
    lcd.setCursor(2,1);
    lcd.print("POWER MODE...");
}
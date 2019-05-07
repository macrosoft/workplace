#ifndef BIG_DIGITS_H
#define BIG_DIGITS_H

#include <LiquidCrystal_I2C.h>

class BigDigitsPrinter {
  public:
    BigDigitsPrinter(LiquidCrystal_I2C *d);
    void begin();
    void print(byte x, float n);
  private:
  byte LT[8] = {B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111};
  byte UB[8] = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
  byte RT[8] = {B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111};
  byte LL[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111};
  byte LB[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
  byte LR[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100};
  byte MB[8] = {B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111};
  LiquidCrystal_I2C *lcd;
  void printDigit(byte x, byte n);
};

BigDigitsPrinter::BigDigitsPrinter(LiquidCrystal_I2C *d): lcd(d) {
}

void BigDigitsPrinter::begin() {
  lcd->createChar(0,LT);
  lcd->createChar(1,UB);
  lcd->createChar(2,RT);
  lcd->createChar(3,LL);
  lcd->createChar(4,LB);
  lcd->createChar(5,LR);
  lcd->createChar(6,MB);
}

void BigDigitsPrinter::print(byte x, float n) {
  printDigit(x, n/10);
  printDigit(x + 4, byte(n)%10);
  lcd->setCursor(x + 7, 1);
  lcd->print(".");
  lcd->print(byte(n*10)%10);
  lcd->print(char(223));
  lcd->print("C");
}

void BigDigitsPrinter::printDigit(byte x, byte n){
  switch (n) {
    case 0:
      lcd->setCursor(x,0);
      lcd->write(0);
      lcd->write(1);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->write(3);
      lcd->write(4);
      lcd->write(5);
      break;
    case 1:
      lcd->setCursor(x,0);
      lcd->write(1);
      lcd->write(2);
      lcd->print(" ");
      lcd->setCursor(x,1);
      lcd->write(4);
      lcd->write(255);
      lcd->write(4);
      break;
    case 2:
      lcd->setCursor(x,0);
      lcd->write(6);
      lcd->write(6);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->write(3);
      lcd->write(4);
      lcd->write(4);
      break;
    case 3:
      lcd->setCursor(x,0);
      lcd->write(6);
      lcd->write(6);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->write(4);
      lcd->write(4);
      lcd->write(5);
      break;
    case 4:
      lcd->setCursor(x,0);
      lcd->write(3);
      lcd->write(4);
      lcd->write(255);
      lcd->setCursor(x, 1);
      lcd->print(" ");
      lcd->print(" ");
      lcd->write(255);
      break;
    case 5:
      lcd->setCursor(x,0);
      lcd->write(3);
      lcd->write(6);
      lcd->write(6);
      lcd->setCursor(x, 1);
      lcd->write(4);
      lcd->write(4);
      lcd->write(5);
      break;
    case 6:
      lcd->setCursor(x,0);
      lcd->write(0);
      lcd->write(6);
      lcd->write(6);
      lcd->setCursor(x, 1);
      lcd->write(3);
      lcd->write(4);
      lcd->write(5);
      break;
    case 7:
      lcd->setCursor(x,0);
      lcd->write(1);
      lcd->write(1);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->print(" ");
      lcd->print(" ");
      lcd->write(255);
      break;
    case 8:
      lcd->setCursor(x,0);
      lcd->write(0);
      lcd->write(6);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->write(3);
      lcd->write(4);
      lcd->write(5);
      break;
    case 9:
      lcd->setCursor(x,0);
      lcd->write(0);
      lcd->write(6);
      lcd->write(2);
      lcd->setCursor(x, 1);
      lcd->write(4);
      lcd->write(4);
      lcd->write(5);
      break;
  }
}

#endif

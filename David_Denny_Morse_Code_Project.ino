// David Denny
// 2026

// Morse Code Translator Device

// Libraries
//   Third Party Libraries:
//     ESP32 Arduino Core: https://github.com/espressif/arduino-esp32
//     ESP32Time by fbiego: https://github.com/fbiego/ESP32Time
//     HC595 by j-bellavance: https://github.com/j-bellavance/HC595
//     Adafruit GFX Library by Adafruit: https://github.com/adafruit/Adafruit-GFX-Library
//     Adafruit SSD1306 by Adafruit: https://github.com/adafruit/Adafruit_SSD1306
#include <string.h>
#include "esp_timer.h"
#include <HC595.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Shift Register Globals
const int chipCount = 4;
const int latchPin  = 17;
const int clockPin  = 16;
const int dataPin   = 4;
HC595 ledArray(chipCount, latchPin, clockPin, dataPin);

// GPIO Globals
const int buzzerPin = 25;
const int clear = 12;
const int tapper = 14;

// Morse Code/Button Globals
int morseCode[6] = {0, 0, 0, 0, 0, 0}; // 1 = short, 2 = long
int morseIndex = 0;
bool pendingCharacter = false;
uint64_t timeoutStart, timeoutEnd;

// OLED Screen Globals
//   On ESP32, Wire uses GPIO 21 (SDA) and GPIO 22 (SCL) by default
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
char OLEDarray[] = ">                                         ";
int lineIndex = 0;
int charIndex[4] = {0, 0, 0, 0};

// ======================= SETUP ==========================

void setup()
{
  delay(50);

  Serial.begin(115200);

  delay(50);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    delay(50);
  }

  display.display();

  delay(2000);

  display.clearDisplay();

  // Test draw
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.println(F(OLEDarray));

  display.display();

  ledArray.reset();

  pinMode(clear, INPUT);
  pinMode(tapper, INPUT);

  ledcAttach(buzzerPin, 600, 8);

  ledcWriteTone(buzzerPin, 0);
}

// ======================== LOOP ==========================

void loop()
{
  // Button Timing and OLED Updates
  if(morseCode[0] != 0 && pendingCharacter == true)
  {
    timeoutEnd = esp_timer_get_time();
  }

  if(pendingCharacter == true &&
    (timeoutEnd - timeoutStart) >= 1000000)
  {
    timeoutEnd = esp_timer_get_time();
    timeoutStart = esp_timer_get_time();
    if(lineIndex == 0)
    {
      if(charIndex[0] < 9)
      {
        OLEDarray[charIndex[0]++ + 1] = translateMorseCode();
      }
    }
    else if(lineIndex == 1)
    {
      if(charIndex[1] < 9)
      {
        OLEDarray[charIndex[1]++ + 11] = translateMorseCode();
      }
    }
    else if(lineIndex == 2)
    {
      if(charIndex[2] < 9)
      {
        OLEDarray[charIndex[2]++ + 21] = translateMorseCode();
      }
    }
    else if(lineIndex == 3)
    {
      if(charIndex[3] < 9)
      {
        OLEDarray[charIndex[3]++ + 31] = translateMorseCode();
      }
    }
    else
    {
      Serial.printf("Invalid index\n");
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F(OLEDarray));
    display.display();
    clearLEDs();

    pendingCharacter = false;
  }

  if(digitalRead(tapper) == HIGH) // Tapper Press
  {
    uint64_t tapperStart = esp_timer_get_time();
    ledcWriteTone(buzzerPin, 600);
    delay(10);
    while(digitalRead(tapper) == HIGH)
    {
      // Wait
    }

    if(morseIndex < 6) // Prevent invalid codes
    {
      uint64_t tapperEnd = esp_timer_get_time();
      if((tapperEnd - tapperStart) < 150000) // Short
      {
        morseCode[morseIndex] = 1;
        if(morseIndex < 4) // Retain LEDs, even if inputting a non-alpha
        {
          updateLEDs(getLEDmask());
        }
        morseIndex++;
        Serial.printf("SHORT   (<150ms)    LEDmask = %08X\n", getLEDmask());
      }
      else if((tapperEnd - tapperStart) >= 150000) // Long
      {
        morseCode[morseIndex] = 2;
        if(morseIndex < 4) // Retain LEDs, even if inputting a non-alpha
        {
          updateLEDs(getLEDmask());
        }
        morseIndex++;
        Serial.printf("LONG    (>150ms)    LEDmask = %08X\n", getLEDmask());
      }
      else
      {
        Serial.printf("Invalid press\n");
      }
      timeoutStart = esp_timer_get_time();
      pendingCharacter = true;
    }
    ledcWriteTone(buzzerPin, 0);
    delay(10);
  }

  delay(10);

  if(digitalRead(clear) == HIGH) // Clear Press
  {
    uint64_t clearStart = esp_timer_get_time();

    while(digitalRead(clear) == HIGH)
    {
      // Wait
    }

    uint64_t clearEnd = esp_timer_get_time();

    // LONG PRESS = clear entire screen
    if((clearEnd - clearStart) >= 1000000)
    {
      // Clear display contents
      for(int i = 0; i < 40; i++)
      {
        OLEDarray[i] = ' ';
      }

      // Reset character indices
      charIndex[0] = 0;
      charIndex[1] = 0;
      charIndex[2] = 0;
      charIndex[3] = 0;

      // Keep cursor active on current line
      if      (lineIndex == 0){OLEDarray[0]  = '>';}
      else if (lineIndex == 1){OLEDarray[10] = '>';}
      else if (lineIndex == 2){OLEDarray[20] = '>';}
      else if (lineIndex == 3){OLEDarray[30] = '>';}
      else    {Serial.printf("Invalid index\n");}

      display.clearDisplay();
      display.setCursor(0,0);
      display.println(F(OLEDarray));
      display.display();
    }

    // SHORT PRESS = change line
    else
    {
      if      (lineIndex == 0){OLEDarray[0]  = ' '; OLEDarray[10] = '>';}
      else if (lineIndex == 1){OLEDarray[10] = ' '; OLEDarray[20] = '>';}
      else if (lineIndex == 2){OLEDarray[20] = ' '; OLEDarray[30] = '>';}
      else if (lineIndex == 3){OLEDarray[30] = ' '; OLEDarray[0]  = '>';}
      else    {Serial.printf("Invalid index\n");}

      lineIndex++;

      if(lineIndex > 3)
      {
        lineIndex = 0;
      }

      display.clearDisplay();

      display.setCursor(0,0);

      display.println(F(OLEDarray));

      display.display();
    }
  }

  delay(10);
}

// ================= HELPER FUNCTIONS ===================

uint32_t getLEDmask() // Function designed for 26 LED design, to apply an LED mask
{
  if      (memcmp(morseCode, (int[]){1, 2, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00018000;} // A
  else if (memcmp(morseCode, (int[]){2, 1, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x00001E00;} // B
  else if (memcmp(morseCode, (int[]){2, 1, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000C18;} // C
  else if (memcmp(morseCode, (int[]){2, 1, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00001C00;} // D
  else if (memcmp(morseCode, (int[]){1, 0, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00010000;} // E
  else if (memcmp(morseCode, (int[]){1, 1, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x000F0000;} // F
  else if (memcmp(morseCode, (int[]){2, 2, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x000004C0;} // G
  else if (memcmp(morseCode, (int[]){1, 1, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x02490000;} // H
  else if (memcmp(morseCode, (int[]){1, 1, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00090000;} // I
  else if (memcmp(morseCode, (int[]){1, 2, 2, 2, 0, 0}, sizeof(int) * 6) == 0) {return 0x0001E000;} // J
  else if (memcmp(morseCode, (int[]){2, 1, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000C10;} // K
  else if (memcmp(morseCode, (int[]){1, 2, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x01118000;} // L
  else if (memcmp(morseCode, (int[]){2, 2, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000480;} // M
  else if (memcmp(morseCode, (int[]){2, 1, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000C00;} // N
  else if (memcmp(morseCode, (int[]){2, 2, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000484;} // O
  else if (memcmp(morseCode, (int[]){1, 2, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x0021C000;} // P
  else if (memcmp(morseCode, (int[]){2, 2, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {return 0x000004C2;} // Q
  else if (memcmp(morseCode, (int[]){1, 2, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00118000;} // R
  else if (memcmp(morseCode, (int[]){1, 1, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00490000;} // S
  else if (memcmp(morseCode, (int[]){2, 0, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000400;} // T
  else if (memcmp(morseCode, (int[]){1, 1, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x000D0000;} // U
  else if (memcmp(morseCode, (int[]){1, 1, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {return 0x00C90000;} // V
  else if (memcmp(morseCode, (int[]){1, 2, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {return 0x0001C000;} // W
  else if (memcmp(morseCode, (int[]){2, 1, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {return 0x00001D00;} // X
  else if (memcmp(morseCode, (int[]){2, 1, 2, 2, 0, 0}, sizeof(int) * 6) == 0) {return 0x00000C11;} // Y
  else if (memcmp(morseCode, (int[]){2, 2, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {return 0x000004E0;} // Z

  else
  {
    Serial.printf("Invalid char\n");
  }

  return 0x00000000;
}

char translateMorseCode()
{
  if      (memcmp(morseCode, (int[]){1, 2, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("A\n"); return 'A';}
  else if (memcmp(morseCode, (int[]){2, 1, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("B\n"); return 'B';}
  else if (memcmp(morseCode, (int[]){2, 1, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("C\n"); return 'C';}
  else if (memcmp(morseCode, (int[]){2, 1, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("D\n"); return 'D';}
  else if (memcmp(morseCode, (int[]){1, 0, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("E\n"); return 'E';}
  else if (memcmp(morseCode, (int[]){1, 1, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("F\n"); return 'F';}
  else if (memcmp(morseCode, (int[]){2, 2, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("G\n"); return 'G';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("H\n"); return 'H';}
  else if (memcmp(morseCode, (int[]){1, 1, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("I\n"); return 'I';}
  else if (memcmp(morseCode, (int[]){1, 2, 2, 2, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("J\n"); return 'J';}
  else if (memcmp(morseCode, (int[]){2, 1, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("K\n"); return 'K';}
  else if (memcmp(morseCode, (int[]){1, 2, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("L\n"); return 'L';}
  else if (memcmp(morseCode, (int[]){2, 2, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("M\n"); return 'M';}
  else if (memcmp(morseCode, (int[]){2, 1, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("N\n"); return 'N';}
  else if (memcmp(morseCode, (int[]){2, 2, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("O\n"); return 'O';}
  else if (memcmp(morseCode, (int[]){1, 2, 2, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("P\n"); return 'P';}
  else if (memcmp(morseCode, (int[]){2, 2, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("Q\n"); return 'Q';}
  else if (memcmp(morseCode, (int[]){1, 2, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("R\n"); return 'R';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("S\n"); return 'S';}
  else if (memcmp(morseCode, (int[]){2, 0, 0, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("T\n"); return 'T';}
  else if (memcmp(morseCode, (int[]){1, 1, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("U\n"); return 'U';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("V\n"); return 'V';}
  else if (memcmp(morseCode, (int[]){1, 2, 2, 0, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("W\n"); return 'W';}
  else if (memcmp(morseCode, (int[]){2, 1, 1, 2, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("X\n"); return 'X';}
  else if (memcmp(morseCode, (int[]){2, 1, 2, 2, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("Y\n"); return 'Y';}
  else if (memcmp(morseCode, (int[]){2, 2, 1, 1, 0, 0}, sizeof(int) * 6) == 0) {Serial.printf("Z\n"); return 'Z';}

  // Numbers
  else if (memcmp(morseCode, (int[]){1, 2, 2, 2, 2, 0}, sizeof(int) * 6) == 0) {Serial.printf("1\n"); return '1';}
  else if (memcmp(morseCode, (int[]){1, 1, 2, 2, 2, 0}, sizeof(int) * 6) == 0) {Serial.printf("2\n"); return '2';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 2, 2, 0}, sizeof(int) * 6) == 0) {Serial.printf("3\n"); return '3';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 1, 2, 0}, sizeof(int) * 6) == 0) {Serial.printf("4\n"); return '4';}
  else if (memcmp(morseCode, (int[]){1, 1, 1, 1, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("5\n"); return '5';}
  else if (memcmp(morseCode, (int[]){2, 1, 1, 1, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("6\n"); return '6';}
  else if (memcmp(morseCode, (int[]){2, 2, 1, 1, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("7\n"); return '7';}
  else if (memcmp(morseCode, (int[]){2, 2, 2, 1, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("8\n"); return '8';}
  else if (memcmp(morseCode, (int[]){2, 2, 2, 2, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("9\n"); return '9';}
  else if (memcmp(morseCode, (int[]){2, 2, 2, 2, 2, 0}, sizeof(int) * 6) == 0) {Serial.printf("0\n"); return '0';}

  // Symbols
  else if (memcmp(morseCode, (int[]){1, 2, 1, 2, 1, 2}, sizeof(int) * 6) == 0) {Serial.printf(".\n"); return '.';}
  else if (memcmp(morseCode, (int[]){2, 2, 1, 1, 2, 2}, sizeof(int) * 6) == 0) {Serial.printf(",\n"); return ',';}
  else if (memcmp(morseCode, (int[]){1, 1, 2, 2, 1, 1}, sizeof(int) * 6) == 0) {Serial.printf("?\n"); return '?';}
  else if (memcmp(morseCode, (int[]){1, 2, 2, 1, 2, 1}, sizeof(int) * 6) == 0) {Serial.printf("@\n"); return '@';}
  else if (memcmp(morseCode, (int[]){2, 1, 1, 2, 1, 0}, sizeof(int) * 6) == 0) {Serial.printf("/\n"); return '/';}

  else
  {
    Serial.printf("Invalid char\n");
    return ' ';
  }
}

void clearLEDs()
{
  morseCode[0] = 0;
  morseCode[1] = 0;
  morseCode[2] = 0;
  morseCode[3] = 0;
  morseCode[4] = 0;
  morseCode[5] = 0;
  morseIndex = 0;
  updateLEDs(0x00000000);
  return;
}

void updateLEDs(uint32_t morseFlow)
{
  for(int i = 0; i < 32; i++)
  {
    if(morseFlow & (1UL << i))
    {
      ledArray.setPin(i, ON);
    }
    else
    {
      ledArray.setPin(i, OFF);
    }
  }
}
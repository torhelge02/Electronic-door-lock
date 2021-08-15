// Inkludere biblioteker
#include <DHT.h>
#include <EEPROM.h>
#include <FastIO.h>
#include <I2CIO.h>
#include <LCD.h>
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_I2C_ByVac.h>
#include <LiquidCrystal_SI2C.h>
#include <LiquidCrystal_SR.h>
#include <LiquidCrystal_SR1W.h>
#include <LiquidCrystal_SR2W.h>
#include <LiquidCrystal_SR3W.h>
#include <SI2CIO.h>
#include <Wire.h>
#include <RFID.h>
#include <DigitalIO.h> // our software SPI library
#include <SPI.h>
#include <SdFat.h> // for the SD card
#include <Adafruit_Fingerprint.h>
#include <Keypad_I2C.h> // I2C Keypad library by Joe Young https://github.com/joeyoung/arduino_keypads
#include <LiquidCrystal_I2C.h>  // I2C LCD Library by Francisco Malpartida https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS3231.h>

//Pin-variabler
const int tageLysPin = 26;
#define ONE_WIRE_BUS 25
const int lockPin = 12;  // Blå = +12V - Hvit = GND
const int holdekretsPin = 11;
const int overridePin = 10;
const int tageModePin = 9;
const int redLED = 8;
const int yellowLED = 7;
const int greenLED = 6;
#define DHTPIN 5 // what digital pin we're connected to
#define CS_SD 4  // pin til SD-leser
#define RST_PIN 3          // pin til RFID-leser
#define SS_PIN 2         // pin til RFID-leser

// Buzzer
const int buzzerPin = 40;  // Pin for buzzer
int times = 150;  // Tider for buzzer-melodi
int times2 = 75;
int times3 = 133;

// Serial til fingeravtrykkleser
#define mySerial Serial1

// Create a file to store the data
SdFat SD;
SdFile myFile;

// Sette MFRC522-pins
RFID rfid(SS_PIN, RST_PIN);

// Definere fingeravtrykkleser
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Variabler for UID
char uidAll[15];
int RFID;
String rxNum;

int ID;  // Fingeravtrykk-ID

// Definere temp/humidity-sensor type og port
#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// Definere tempsensor 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Definere klokke
DS3231 clock;
bool century = false;
bool h12Flag;
bool pmFlag;

// Definere I2C-porter
#define lcd_addr 0x27     // I2C address of typical I2C LCD Backpack
#define keypad_addr 0x24  // I2C address of I2C Expander module

// LCD Pins to I2C LCD Backpack - These are default for HD44780 LCD's
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

// Create instance for LCD called: i2c_lcd
LiquidCrystal_I2C i2c_lcd(lcd_addr, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

// Tagemode variabler
const int tageLysMode = 2;
const int tageLysTotal = 10;

unsigned long passord = 0;
unsigned long masterPass = 0;
unsigned int oldPass = 0;
int pozisyon = 0;
bool masterCard = 0;
bool checkCode = 0;
int pressedCode;
char whichKeyG;

// Verdier fra funksjoner
float humidity = 0;
float temp = 0;
int pulseInValue = 0;
bool runLoop = 1;
bool firstPass = 1;

// Relé
bool releON = 0;
bool releOFF = 1;

// Automatisk av, tidsvariabel
unsigned long offTime = 0;

// Variabler til dato og tid
int dayRTC;
int monthRTC;
int yearRTC;
int hourRTC;
int minuteRTC;
int secondRTC;
int DoWRTC;

// Define the keypad pins
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Digitran keypad, bit numbers of PCF8574 i/o port
byte rowPins[ROWS] = {4, 5, 6, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {0, 1, 2, 3}; //connect to the column pinouts of the keypad

// Create instance of the Keypad name I2C_Keypad and using the PCF8574 chip
Keypad_I2C I2C_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, keypad_addr, PCF8574 );

void setup() {
  // Starte diverse tileggstjenester
  Serial.begin(9600);
  dht.begin();  // Starte temp/humidity-sensor
  Wire.begin();
  rfid.init(); // initilize the RFID module
  SPI.begin();
  I2C_Keypad.begin();  // Starte keypad
  finger.begin(57600);  // Starte fingeravtrykksensor
  sensors.begin();  // Starte tempsensor

  i2c_lcd.begin (16, 2); // Our LCD is a 16x2, change for your LCD if needed

  clock.setClockMode(false); // set to 24h

  // LCD Backlight ON
  i2c_lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  i2c_lcd.setBacklight(HIGH);
  i2c_lcd.clear(); // Clear the LCD screen

  // Sette pinMode og digitalWrite:
  pinMode(holdekretsPin, OUTPUT);
  digitalWrite(holdekretsPin, releON);
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, releOFF);
  pinMode(redLED, OUTPUT);
  digitalWrite(redLED, LOW);
  pinMode(yellowLED, OUTPUT);
  digitalWrite(yellowLED, LOW);
  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED, LOW);
  pinMode(tageLysPin, OUTPUT);
  digitalWrite(tageLysPin, LOW);

  // Hente passord
  for (int eepromAddr = 0; eepromAddr < 4; eepromAddr++) {
    passord = passord * 10 + EEPROM.read(eepromAddr);
  }
  while (passord > 9999) {
    passord = passord / 10;
  }

  // Hente masterpassord
  for (int eepromAddr = 4; eepromAddr < 8; eepromAddr++) {
    masterPass = masterPass * 10 + EEPROM.read(eepromAddr);
    Serial.println(masterPass);
  }
  while (masterPass > 9999) {
    masterPass = masterPass / 10;
  }
  Serial.println(passord);

  checkReaders();  // Sjekke sensorer

  i2c_lcd.clear();
}



void loop() {
  //Tage mode: (tageMode == lockdown mode)
  if (digitalRead(tageModePin) == LOW) {
    tageMode();
  }

  //Skru av etter 30 sek uten aktivitet
  if (millis() - offTime > 30000) {
    turnOFF();
    digitalWrite(holdekretsPin, releOFF);
  }

  led(2);

  masterCard = 0;

  // Put value of pressed key on keypad in key variable
  whichKeyG = I2C_Keypad.getKey(); // Define which key is pressed with getKey

  runLoop = 1;
  firstPass = 1;
  setLocked (true);
  i2c_lcd.setCursor(1, 0);
  i2c_lcd.print("Use card, code");
  i2c_lcd.setCursor(1, 1);
  i2c_lcd.print("or fingerprint");

  checkTemp();

  getCode();

  checkReaders();

  checkCard();

  checkFingerprint();

  checkPin();

  checkOverride();

  showTime();
}

void tageMode() {
  setLocked(true);

  //Vise "LOCKDOWN" på skjerm
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("*** LOCKDOWN ***");

  for (int tageLys = 1; tageLys <= tageLysMode; tageLys++) { //Skru på tageLys
    digitalWrite(tageLysPin, HIGH);
    delay(100);
    digitalWrite(tageLysPin, LOW);
    delay(100);
  }

  while (digitalRead(tageModePin) == LOW) { // Når tagemode er på
    led(3);
    delay(300);
    led(0);
    delay(1000);

    checkOverride();
  }
  for (int tageLysAv = 1; tageLysAv <= (tageLysTotal - tageLysMode); tageLysAv++) { //Skru av tageLys
    digitalWrite(tageLysPin, HIGH);
    delay(100);
    digitalWrite(tageLysPin, LOW);
    delay(100);
  }
  i2c_lcd.clear();
  offTime = millis();
}

void checkPin() {
  if (checkCode == 1) {
    //unsigned long tempCode = 59 * now.day() * 25 / now.month() / now.year() * 98 * 88 / 78 * 96 / 48 * 86 / 54 * 79;

    /*while (tempCode > 9999) {
      tempCode = tempCode / 10;
      }*/
    Serial.println(passord);
    //Serial.println(tempCode);
    if (pressedCode == masterPass) {
      masterMode();
    }
    else if (pressedCode == passord /*|| pressedCode == tempCode*/) {
      unlockDoor();
    }
    else {
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print("*Wrong password*");

      for (offTime = millis(); (offTime + 1000) > millis() && runLoop == 1;) {
        char whichKeyG = I2C_Keypad.getKey(); // Define which key is pressed with getKey

        checkOverride();

        if (whichKeyG == '#') {
          runLoop = 0;
        }
        else if (isDigit(whichKeyG)) {
          i2c_lcd.clear();
          getCode();
        }
      }
      i2c_lcd.clear();
    }
    checkCode = 0;
  }
  else {
    led(2);
  }
}

void checkReaders() {
  if (!SD.begin(CS_SD)) { // Sjekke om SD-leser fungerer
    i2c_lcd.clear();
    i2c_lcd.setCursor(0, 0);
    i2c_lcd.print("Error with SD");
    i2c_lcd.setCursor(0, 1);
    i2c_lcd.print("reader");

    Serial.println("SD error");

    while (!SD.begin(CS_SD)) {
      led(3);
      delay(250);
      led(0);
      delay(250);
    }
    i2c_lcd.clear();
  }

  if (!finger.verifyPassword()) { // Sjekke om SD-leser fungerer
    i2c_lcd.clear();
    i2c_lcd.setCursor(0, 0);
    i2c_lcd.print("Error with");
    i2c_lcd.setCursor(0, 1);
    i2c_lcd.print("finger reader");

    Serial.println("Fingerprint error");

    while (!finger.verifyPassword()) {
      led(3);
      delay(250);
      led(0);
      delay(250);
    }
    i2c_lcd.clear();
  }

  /*if (!RTC.begin()) {
    i2c_lcd.clear();
    i2c_lcd.setCursor(0, 0);
    i2c_lcd.print("Error with RTC");
    i2c_lcd.setCursor(0, 1);
    i2c_lcd.print("clock");

    while (!RTC.begin()) {
      led(3);
      delay(250);
      led(0);
      delay(250);
    }
    i2c_lcd.clear();
    }*/
}

void checkCard() {
  // Sjekke om kortet er lest riktig
  if (! rfid.isCard()) {
    return;
  }
  if (! rfid.readCardSerial()) {
    return;
  }

  getUID();  // Hente UID fra kort

  Serial.println(uidAll);

  if (SD.exists(uidAll)) { // Sjekke om kortet eksisterer i databasen
    myFile.open(uidAll);  // Åpne filen til kortet
    if (myFile) {
      char cardContent[0];
      while (myFile.available()) {
        cardContent[0] = myFile.read();
        Serial.println(myFile.read());
        Serial.println(cardContent);
      }
      if (cardContent[0] == 'm') { // Sjekke om kortet er et masterkort
        masterCard = 1;
        masterMode();  // Starte mastermode
      }
      else {
        unlockDoor();  // Låse opp dør
      }
      myFile.close();  // Lukke fil
    }
  }
}

void checkFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  ID = finger.fingerID;
  logAccess("0", 0);
  unlockDoor();
}

void checkTemp() {
  if (whichKeyG == 'A') {
    i2c_lcd.clear();
    i2c_lcd.setCursor(0, 0);
    i2c_lcd.print("Requesting temp");
    sensors.requestTemperatures(); // Send the command to get temperature readings

    humidity = dht.readHumidity();
    temp = sensors.getTempCByIndex(0); // Why "byIndex"?
    // You can have more than one DS18B20 on the same bus.
    // 0 refers to the first IC on the wire
    //delay(250);  // Forsinkelse for å la Arduino lese sensor

    i2c_lcd.clear();
    i2c_lcd.setCursor(0, 0);
    i2c_lcd.print("Temp:");
    i2c_lcd.setCursor(6, 0);
    i2c_lcd.print(temp);
    i2c_lcd.setCursor(11, 0);
    i2c_lcd.print("C");

    i2c_lcd.setCursor(0, 1);
    i2c_lcd.print("Humidity:");
    i2c_lcd.setCursor(10, 1);
    i2c_lcd.print(humidity);
    i2c_lcd.setCursor(15, 1);
    i2c_lcd.print("%");

    for (offTime = millis(); (offTime + 3000) > millis() && runLoop == 1;) {
      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

      checkOverride();

      if (whichKey == '#') { // Hvis avbryt blir trykket inn
        runLoop = 0;
      }
    }
    i2c_lcd.clear();
  }
}

void getCode() {
  if (isDigit(whichKeyG)) {  // Lyse med grønn LED når en tast blir trykket inn
    led(3);
    offTime = millis();
    i2c_lcd.clear();
    i2c_lcd.setCursor(6, 0);
    i2c_lcd.print("*");
    pressedCode = whichKeyG - '0';
    int intWhichKey;
    pozisyon = 1;
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
    while (whichKey == NO_KEY) {
      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
      if (whichKey) {
        intWhichKey = whichKey - '0';
        break;
      }
      checkOverride();
    }
    pressedCode = pressedCode * 10 + intWhichKey;
    i2c_lcd.setCursor(7, 0);
    i2c_lcd.print("*");
    pozisyon = 2;
    while (whichKey == NO_KEY) {
      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
      if (whichKey) {
        intWhichKey = whichKey - '0';
        break;
      }
    }

    pressedCode = pressedCode * 10 + intWhichKey;
    i2c_lcd.setCursor(8, 0);
    i2c_lcd.print("*");
    pozisyon = 3;
    while (whichKey == NO_KEY) {
      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
      if (whichKey) {
        intWhichKey = whichKey - '0';
        break;
      }
    }

    pressedCode = pressedCode * 10 + intWhichKey;
    i2c_lcd.setCursor(9, 0);
    i2c_lcd.print("*");
    checkCode = 1;
  }
}

void checkOverride() {
  if (digitalRead(overridePin) == LOW) {
    Serial.println("Override!");
    unlockDoor();
  }
  else {
    setLocked(true);
  }
}

void showTime() {  // Vise tiden på skjermen
  if (whichKeyG == 'B') {
    i2c_lcd.clear();

    if (clock.getDate() < 10) {
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print("0");
      i2c_lcd.setCursor(1, 0);
      i2c_lcd.print(clock.getDate(), DEC);
    }
    else {
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print(clock.getDate(), DEC);
    }

    i2c_lcd.setCursor(2, 0);
    i2c_lcd.print("/");

    if (clock.getMonth(century) < 10) {
      i2c_lcd.setCursor(3, 0);
      i2c_lcd.print("0");
      i2c_lcd.setCursor(4, 0);
      i2c_lcd.print(clock.getMonth(century), DEC);
    }
    else {
      i2c_lcd.setCursor(4, 0);
      i2c_lcd.print(clock.getMonth(century), DEC);
    }

    i2c_lcd.setCursor(5, 0);
    i2c_lcd.print("/");
    i2c_lcd.setCursor(6, 0);
    i2c_lcd.print(2000 + clock.getYear(), DEC);

    if (clock.getHour(h12Flag, pmFlag) < 10) {
      i2c_lcd.setCursor(11, 0);
      i2c_lcd.print("0");
      i2c_lcd.setCursor(12, 0);
      i2c_lcd.print(clock.getHour(h12Flag, pmFlag), DEC); //24-hr
    }
    else {
      i2c_lcd.setCursor(11, 0);
      i2c_lcd.print(clock.getHour(h12Flag, pmFlag), DEC); //24-hr
    }

    i2c_lcd.setCursor(13, 0);
    i2c_lcd.print(":");

    if (clock.getMinute() < 10) {
      i2c_lcd.setCursor(14, 0);
      i2c_lcd.print("0");
      i2c_lcd.setCursor(15, 0);
      i2c_lcd.print(clock.getMinute(), DEC);
    }
    else {
      i2c_lcd.setCursor(14, 0);
      i2c_lcd.print(clock.getMinute(), DEC);
    }

    for (offTime = millis(); (offTime + 3000) > millis() && runLoop == 1;) {
      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

      checkOverride();

      if (whichKey == '#') { // Hvis avbryt blir trykket inn
        runLoop = 0;
      }
    }
    i2c_lcd.clear();
  }
}

void setLocked(int locked) {
  if (locked) {
    led(2);
    digitalWrite(lockPin, releOFF);
  }
  else {
    led(1);
    digitalWrite(lockPin, releON);
  }
}

void masterMode() {
  led(1);
  firstPass = 1;
  runLoop = 1;
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Entered");
  i2c_lcd.setCursor(0, 1);
  i2c_lcd.print("master mode");
  delay(1000);

  // Skriv meny på skjerm
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("A: Code B: Card");
  i2c_lcd.setCursor(0, 1);
  i2c_lcd.print("C: Print D: More");

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) {
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    checkOverride();

    if (whichKey == '#') { // Hvis avbryt blir trykket inn
      runLoop = 0;
    }

    if (whichKey == 'A') { // Menyalternativ A
      firstPass = 1;
      // Skriv ny meny på skjerm
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print("A: Door code");
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print("B: Master code");
      offTime = millis();

      for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) {
        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

        checkOverride();

        if (whichKey == '#') { // Hvis avbryt blir trykket inn
          runLoop = 0;
        }

        if (whichKey == 'A') { // Ny kode
          // Skrive på skjerm
          i2c_lcd.clear();
          i2c_lcd.setCursor(0, 0);
          i2c_lcd.print("Enter new code:");
          i2c_lcd.setCursor(0, 1);
          i2c_lcd.print("(4 digits)");
          offTime = millis();

          for (int doorAddr = 0; doorAddr < 4  && runLoop == 1;) { // Endring av passord
            char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

            checkOverride();

            if (doorAddr == 0 && firstPass == 1) {
              oldPass = passord;
              EEPROM.write(doorAddr + 10, EEPROM.read(doorAddr));  // Sikkerhetskopiere passord
              EEPROM.write(doorAddr + 11, EEPROM.read(doorAddr + 1));
              EEPROM.write(doorAddr + 12, EEPROM.read(doorAddr + 2));
              EEPROM.write(doorAddr + 13, EEPROM.read(doorAddr + 3));
              passord = 0;
              offTime = millis();
              firstPass = 0;
            }
            if (whichKey != NO_KEY && isDigit(whichKey) && whichKey != '#' && whichKey != '*') {
              EEPROM.write(doorAddr, whichKey - '0');
              i2c_lcd.setCursor(11 + doorAddr, 1);
              i2c_lcd.print("*");
              doorAddr++;
              offTime = millis();
              if (doorAddr > 3) {
                // Hente passord
                for (int eepromAddr = 0; eepromAddr < 4; eepromAddr++) {
                  passord = passord * 10 + EEPROM.read(eepromAddr);
                }
                runLoop = 0;
              }
            }
            if (whichKey == '#' || offTime + 10000 < millis()) { // Tilbakestille koden hvis prosessen blir avbrutt
              passord = oldPass;
              doorAddr = 0;
              EEPROM.write(doorAddr, EEPROM.read(doorAddr + 10));  // Tilbakestille masterPass
              EEPROM.write(doorAddr + 1, EEPROM.read(doorAddr + 11));
              EEPROM.write(doorAddr + 2, EEPROM.read(doorAddr + 12));
              EEPROM.write(doorAddr + 3, EEPROM.read(doorAddr + 13));
              offTime = millis();
              runLoop = 0;
            }
          }
        }
        if (whichKey == 'B') { // Endre masterkode
          firstPass = 1;

          offTime = millis();
          pressedCode = 0;
          if (masterCard == 0 && runLoop == 1) { // Hvis masterkort ikke ble brukt til å åpne mastermode
            i2c_lcd.clear();
            i2c_lcd.setCursor(0, 0);
            i2c_lcd.print("Scan master card");

            while (! rfid.isCard() || ! rfid.readCardSerial()) {
              char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
              if ((offTime + 10000) < millis() || whichKey == '#') {
                offTime = millis();
                runLoop = 0;
                break;
              }
            }

            getUID();  // Hente UID fra kort

            if (SD.exists(uidAll)) { // Sjekke om kortet eksisterer i databasen
              myFile.open(uidAll);  // Åpne filen til kortet
              if (myFile) {
                char cardContent[0];
                while (myFile.available()) {
                  cardContent[0] = myFile.read();
                }
                if (cardContent[0] != 'm') { // Sjekke om kortet er et masterkort
                  offTime = millis();
                  runLoop = 0;
                  break;
                }
                myFile.close();  // Lukke fil
              }
            }
          }

          else if (masterCard == 1 && runLoop == 1) { // Hvis masterkort ble brukt til å åpne mastermode
            i2c_lcd.clear();
            i2c_lcd.setCursor(0, 0);
            i2c_lcd.print("Tast masterkode:");
            while (pressedCode != masterPass && runLoop == 1) {
              getCode();
              if (pressedCode != masterPass && checkCode == 1) {
                i2c_lcd.clear();
                i2c_lcd.setCursor(0, 0);
                i2c_lcd.print("Feil kode");
                delay(500);
                i2c_lcd.clear();
                i2c_lcd.setCursor(0, 0);
                i2c_lcd.print("Tast masterkode:");
              }
              if ((offTime + 10000) > millis()) {
                if (whichKey == '#') {
                  runLoop = 0;
                  break;
                }
              }
              else { // Bryte loop hvis det går over 10sek
                offTime = millis();
                runLoop = 0;
                break;
              }
              checkCode = 0;
            }
          }
          else {
            offTime = millis();
            runLoop = 0;
            break;
          }

          // Skrive på skjerm
          i2c_lcd.clear();
          i2c_lcd.setCursor(0, 0);
          i2c_lcd.print("Enter new code:");
          i2c_lcd.setCursor(0, 1);
          i2c_lcd.print("(4 digits)");
          offTime = millis();

          for (int doorAddr = 4; doorAddr < 8  && runLoop == 1;) { // Endring av mastekode
            char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

            checkOverride();

            if (doorAddr == 4 && firstPass == 1) {
              oldPass = masterPass;
              EEPROM.write(doorAddr + 10, EEPROM.read(doorAddr));  // Sikkerhetskopiere passord
              EEPROM.write(doorAddr + 11, EEPROM.read(doorAddr + 1));
              EEPROM.write(doorAddr + 12, EEPROM.read(doorAddr + 2));
              EEPROM.write(doorAddr + 13, EEPROM.read(doorAddr + 3));
              masterPass = 0;
              offTime = millis();
              firstPass = 0;
            }
            if (whichKey != NO_KEY && isDigit(whichKey) && whichKey != '#' && whichKey != '*') {
              EEPROM.write(doorAddr, whichKey - '0');
              i2c_lcd.setCursor(7 + doorAddr, 1);
              i2c_lcd.print("*");
              doorAddr++;
              offTime = millis();
              if (doorAddr > 7) {
                // Hente masterpassord
                for (int eepromAddr = 4; eepromAddr < 8; eepromAddr++) {
                  masterPass = masterPass * 10 + EEPROM.read(eepromAddr);
                }
                runLoop = 0;
              }
            }
            if (whichKey == '#' || offTime + 10000 < millis()) { // Tilbakestille koden hvis prosessen blir avbrutt
              masterPass = oldPass;
              doorAddr = 4;
              EEPROM.write(doorAddr, EEPROM.read(doorAddr + 10));  // Tilbakestille passord
              EEPROM.write(doorAddr + 1, EEPROM.read(doorAddr + 11));
              EEPROM.write(doorAddr + 2, EEPROM.read(doorAddr + 12));
              EEPROM.write(doorAddr + 3, EEPROM.read(doorAddr + 13));
              offTime = millis();
              runLoop = 0;
            }
          }
        }
      }
      i2c_lcd.clear();
    }
    if (whichKey == 'B') { // Endre kort
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print("A: Add B: Remove");
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print("C: Master card");
      offTime = millis();

      for (offTime = millis(); offTime + 10000 > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

        checkOverride();

        if (whichKey == "#") { // Hvis avbryt blir trykket inn
          runLoop = 0;
        }

        if (whichKey == 'A') { // Legg til kort
          i2c_lcd.clear();
          i2c_lcd.setCursor(0, 0);
          i2c_lcd.print("Please wait...");
          offTime = millis();

          for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
            char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

            if (whichKey == '#') { // Hvis avbryt blir trykket inn
              runLoop = 0;
            }

            getUIDFunction();  // Hente UID for funksjon

            myFile.open(uidAll, FILE_WRITE);  // Lagre kort til database

            if (myFile) { // Hvis kortet ble lagret
              myFile.close();  // Lukk filen til kortet

              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("New card saved");
              led(1);
              delay(1000);

              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("UID:");
              i2c_lcd.setCursor(5, 0);
              i2c_lcd.print(uidAll);
              i2c_lcd.setCursor(0, 1);
              i2c_lcd.print("Press # to exit");
              while (whichKey != '#') {
                char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                checkOverride();
              }
            }
            else { // Hvis kortet ikke ble lagret
              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("Error saving");
              i2c_lcd.setCursor(0, 1);
              i2c_lcd.print("card");
              led(3);
              delay(1000);

              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("UID:");
              i2c_lcd.setCursor(5, 0);
              i2c_lcd.print(uidAll);
              i2c_lcd.setCursor(0, 1);
              i2c_lcd.print("Press # to exit");
              while (whichKey != '#') {
                char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                checkOverride();
              }
            }
          }
        }
        if (whichKey == 'B') { // Remove card
          // Skrive på skjerm
          i2c_lcd.clear();
          i2c_lcd.setCursor(0, 0);
          i2c_lcd.print("A: Scan card");
          i2c_lcd.setCursor(0, 1);
          i2c_lcd.print("B: Enter card");
          offTime = millis();

          for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
            char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

            checkOverride();

            if (whichKey == '#') { // Hvis avbryt blir trykket inn
              runLoop = 0;
            }

            if (whichKey == 'A') { // Skanne kort
              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("Please wait...");
              offTime = millis();

              for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
                char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                checkOverride();

                getUIDFunction();  // Hente UID for funksjon

                SD.remove(uidAll);  // Fjerne kort fra database

                if (!SD.exists(uidAll)) {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Card removed");
                  led(1);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("UID:");
                  i2c_lcd.setCursor(5, 0);
                  i2c_lcd.print(uidAll);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                }
                else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Unable to");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("delete card");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("UID:");
                  i2c_lcd.setCursor(5, 0);
                  i2c_lcd.print(uidAll);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                  return;  // Avbryte loop
                }
              }
            }
            if (whichKey == 'B') { // Slette innstastet kort
              // Printe på skjerm
              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("Please wait...");
              offTime = millis();

              for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
                char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                checkOverride();

                if (whichKey == '#') { // Hvis avbryt blir trykket inn
                  runLoop = 0;
                }

                enterUID();

                rxNum.toCharArray(uidAll, 13);

                delay(200);
                if (SD.exists(uidAll)) {
                  SD.remove(uidAll);  // Fjerne kort fra database
                }
                else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("The card doesn't");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("exist");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("UID:");
                  i2c_lcd.setCursor(5, 0);
                  i2c_lcd.print(uidAll);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                  return;  // Avbryte loop
                }

                if (!SD.exists(uidAll)) { // Sjekke om kort eksisterer
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Card removed");
                  led(1);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("UID:");
                  i2c_lcd.setCursor(5, 0);
                  i2c_lcd.print(uidAll);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                }
                else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("The card doesn't");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("exist");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("UID:");
                  i2c_lcd.setCursor(5, 0);
                  i2c_lcd.print(uidAll);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                  return;  // Avbryte loop
                }
              }
            }
            if (whichKey == 'C') { // Masterkort
              // Skrive på skjerm
              i2c_lcd.clear();
              i2c_lcd.setCursor(0, 0);
              i2c_lcd.print("A: Add");
              i2c_lcd.setCursor(0, 1);
              i2c_lcd.print("B: Remove");
              offTime = millis();

              for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
                char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                checkOverride();

                if (whichKey == '#') { // Hvis avbryt blir trykket inn
                  runLoop = 0;
                }

                if (whichKey == 'A') { // Legge til masterkort
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Please wait...");
                  offTime = millis();

                  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();

                    if (whichKey == '#') { // Hvis avbryt blir trykket inn

                      runLoop = 0;
                    }

                    // Printe på skjerm
                    i2c_lcd.clear();
                    i2c_lcd.setCursor(0, 0);
                    i2c_lcd.print("Scan card");
                    offTime = millis();

                    getUIDFunction();  // Hente UID for funksjoner

                    myFile.open(uidAll, FILE_WRITE);  // Åpne filen til kortet

                    if (myFile) { // Hvis filen ble åpnet
                      char cardContent[0];
                      while (myFile.available()) {
                        cardContent[0] = myFile.read();
                      }
                      if (cardContent[0] == 'm') { // Sjekke om kortet er et masterkort
                      }
                      else {
                        myFile.print("m");  // Skriv "m" til filen til kortet
                      }
                      myFile.close();  // Lukk filen til kortet

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Master card");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("added");
                      led(1);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("UID:");
                      i2c_lcd.setCursor(5, 0);
                      i2c_lcd.print(uidAll);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                    }
                    else { // Hvis filen ikke ble åpnet
                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Unable to add");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("master card");
                      led(3);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("UID:");
                      i2c_lcd.setCursor(5, 0);
                      i2c_lcd.print(uidAll);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                      return;  // Avbryte loop
                    }
                  }
                }
                if (whichKey == 'B') { // Fjerne masterkort
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Please wait...");
                  offTime = millis();

                  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();

                    if (whichKey == '#') { // Hvis avbryt blir trykket inn
                      runLoop = 0;
                    }

                    getUIDFunction();  // Hente UID for funksjoner

                    SD.remove(uidAll);  // Fjern kort fra database
                    myFile.open(uidAll, FILE_WRITE);  // Lagre kort til databasen igjen (den er da blank)

                    if (myFile) { // Hvis filen er lagret
                      myFile.close();  // Lukk filen

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Master card");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("deleted");
                      led(1);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.print("It's still able");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("to unlock door");
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("UID:");
                      i2c_lcd.setCursor(5, 0);
                      i2c_lcd.print(uidAll);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                    }
                    else { // Hvis filen ikke er lagret
                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Unable to delete");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("master card");
                      led(3);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("UID:");
                      i2c_lcd.setCursor(5, 0);
                      i2c_lcd.print(uidAll);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                      return;  // Avbryte loop
                    }

                  }
                }
              }
            }
          }
          if (whichKey == '#') { // Hvis avbryt blir trykket inn
            runLoop = 0;
          }
        }
        i2c_lcd.clear();
      }
      if (whichKey == 'C') {  // Fingeravtrykk
        // Skrive på skjerm
        i2c_lcd.clear();
        i2c_lcd.setCursor(0, 0);
        i2c_lcd.print("A: Add B: Remove");
        i2c_lcd.setCursor(0, 1);
        i2c_lcd.print("C: Remove all");
        offTime = millis();

        for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
          char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

          checkOverride();

          if (whichKey == '#') { // Hvis avbryt blir trykket inn
            runLoop = 0;
          }

          if (whichKey == 'A') { // Legge til fingeravtrykk
            getNumber();

            i2c_lcd.clear();
            i2c_lcd.setCursor(0, 0);
            i2c_lcd.print("Scan finger");
            offTime = millis();

            for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
              char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

              checkOverride();

              if (whichKey == '#') { // Hvis avbryt blir trykket inn
                runLoop = 0;
              }

              if (ID > 0 && ID < 128) {
                int p = -1;
                while (p != FINGERPRINT_OK) {
                  checkOverride();

                  p = finger.getImage();
                  switch (p) {
                    case FINGERPRINT_OK:
                      break;
                    default:
                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Error saving");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("fingerprint");
                      led(3);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("ID:");
                      i2c_lcd.setCursor(4, 0);
                      i2c_lcd.print(ID);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                      runLoop = 0;
                      break;
                  }
                }

                // OK success!

                p = finger.image2Tz(1);
                switch (p) {
                  case FINGERPRINT_OK:
                    break;
                  default:
                    i2c_lcd.clear();
                    i2c_lcd.setCursor(0, 0);
                    i2c_lcd.print("Error saving");
                    i2c_lcd.setCursor(0, 1);
                    i2c_lcd.print("fingerprint");
                    led(3);
                    delay(1000);

                    i2c_lcd.clear();
                    i2c_lcd.setCursor(0, 0);
                    i2c_lcd.print("ID:");
                    i2c_lcd.setCursor(4, 0);
                    i2c_lcd.print(ID);
                    i2c_lcd.setCursor(0, 1);
                    i2c_lcd.print("Press # to exit");
                    while (whichKey != '#') {
                      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                      checkOverride();
                    }
                    runLoop = 0;
                    return p;
                }

                i2c_lcd.clear();
                i2c_lcd.setCursor(0, 0);
                i2c_lcd.print("Remove finger");
                offTime = millis();
                delay(2000);
                p = 0;
                while (p != FINGERPRINT_NOFINGER) {
                  p = finger.getImage();

                  checkOverride();
                }
                p = -1;
                i2c_lcd.clear();
                i2c_lcd.setCursor(0, 0);
                i2c_lcd.print("Scan finger");
                offTime = millis();
                while (p != FINGERPRINT_OK) {
                  checkOverride();

                  p = finger.getImage();
                  switch (p) {
                    case FINGERPRINT_OK:
                      break;
                    default:
                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("Error saving");
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("fingerprint");
                      led(3);
                      delay(1000);

                      i2c_lcd.clear();
                      i2c_lcd.setCursor(0, 0);
                      i2c_lcd.print("ID:");
                      i2c_lcd.setCursor(4, 0);
                      i2c_lcd.print(ID);
                      i2c_lcd.setCursor(0, 1);
                      i2c_lcd.print("Press # to exit");
                      while (whichKey != '#') {
                        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                        checkOverride();
                      }
                      runLoop = 0;
                      break;
                  }
                }

                // OK success!

                p = finger.image2Tz(2);
                switch (p) {
                  case FINGERPRINT_OK:
                    break;
                  default:
                    i2c_lcd.clear();
                    i2c_lcd.setCursor(0, 0);
                    i2c_lcd.print("Error saving");
                    i2c_lcd.setCursor(0, 1);
                    i2c_lcd.print("fingerprint");
                    led(3);
                    delay(1000);

                    i2c_lcd.clear();
                    i2c_lcd.setCursor(0, 0);
                    i2c_lcd.print("ID:");
                    i2c_lcd.setCursor(4, 0);
                    i2c_lcd.print(ID);
                    i2c_lcd.setCursor(0, 1);
                    i2c_lcd.print("Press # to exit");
                    while (whichKey != '#') {
                      char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                      checkOverride();
                    }
                    runLoop = 0;
                    return p;
                }

                // OK converted!

                p = finger.createModel();
                if (p == FINGERPRINT_OK) {
                } else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Error saving");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("fingerprint");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("ID:");
                  i2c_lcd.setCursor(4, 0);
                  i2c_lcd.print(ID);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                  runLoop = 0;
                  return p;
                }

                p = finger.storeModel(ID);
                if (p == FINGERPRINT_OK) {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("New fingerprint");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("saved");
                  led(1);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("ID:");
                  i2c_lcd.setCursor(4, 0);
                  i2c_lcd.print(ID);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                } else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Error saving");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("fingerprint");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("ID:");
                  i2c_lcd.setCursor(4, 0);
                  i2c_lcd.print(ID);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                  runLoop = 0;
                  return p;
                }
              }
            }
          }
          if (whichKey == 'B') {  // Slette fingeravtrykk
            getNumber();
            for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
              char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

              checkOverride();

              if (whichKey == '#') { // Hvis avbryt blir trykket inn
                runLoop = 0;
              }

              if (ID > 0 && ID < 128) {
                uint8_t p = -1;

                p = finger.deleteModel(ID);

                if (p == FINGERPRINT_OK) {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Fingerprint");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("deleted: ");
                  led(1);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("ID:");
                  i2c_lcd.setCursor(4, 0);
                  i2c_lcd.print(ID);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                }
                else {
                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("Error deleting");
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("fingerprint");
                  led(3);
                  delay(1000);

                  i2c_lcd.clear();
                  i2c_lcd.setCursor(0, 0);
                  i2c_lcd.print("ID:");
                  i2c_lcd.setCursor(4, 0);
                  i2c_lcd.print(ID);
                  i2c_lcd.setCursor(0, 1);
                  i2c_lcd.print("Press # to exit");
                  while (whichKey != '#') {
                    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

                    checkOverride();
                  }
                }
              }
            }
          }
          if (whichKey == 'C') {
            finger.emptyDatabase();
            i2c_lcd.clear();
            i2c_lcd.setCursor(0, 0);
            i2c_lcd.print("All fingerprints");
            i2c_lcd.setCursor(0, 1);
            i2c_lcd.print("deleted");
            led(1);
            delay(1000);
          }
        }
      }
      i2c_lcd.clear();
    }
    if (whichKey == 'D') { // Unlock door
      // Skriv meny på skjerm
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 0);
      i2c_lcd.print("A: Time");
      i2c_lcd.setCursor(8, 0);
      i2c_lcd.print("B: Door");
      i2c_lcd.setCursor(5, 1);
      i2c_lcd.print("D: Turn off");
      offTime = millis();

      for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) {
        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

        checkOverride();

        if (whichKey == '#') { // Hvis avbryt blir trykket inn
          runLoop = 0;
        }

        if (whichKey == 'A') { // Menyalternativ A
          setTime();  // Endre tid
          runLoop = 0;
        }


        if (whichKey == 'B') { // Menyalternativ B
          unlockDoor();  // Unlock door
          runLoop = 0;
        }

        if (whichKey == 'D') { // Menyalternativ D
          turnOFF();  // Turn off lock
          runLoop = 0;
        }
      }
      i2c_lcd.clear();
    }
  }
  i2c_lcd.clear();
}

void unlockDoor() { // Låse opp dør
  setLocked(false);
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("*** Verified ***");
  offTime = millis();
  cucaracha();

  for (; (offTime + 3000) > millis() && runLoop == 1 || digitalRead(overridePin) == LOW;) {
    char whichKeyG = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (whichKeyG == '#') {
      runLoop = 0;
    }
    else if (isDigit(whichKeyG)) {
      i2c_lcd.clear();
      getCode();
    }
  }

  i2c_lcd.clear();
  offTime = millis();
  setLocked(true);
}

void logAccess(String data, int card) {
  myFile.open("log.txt", FILE_WRITE);  // Åpne filen

  if (myFile) {
    if (card == 1) {
      myFile.print("NFC UID: ");
      myFile.println(data);  // Skrive data til filen
    }
    else if (card == 0) {
      myFile.print("Fingerprint ID: ");
      myFile.println(ID);
    }
  }
  myFile.close();
}

void getUIDFunction() {  // Hente UID for funksjoner
  String rxNumOld = rxNum;
  while (! rfid.isCard()) { // Når kort ikke er presentert
    if ((offTime + 10000) < millis()) { // Hvis avbryt blir trykket inn
      offTime = millis();
      runLoop = 1;  // Sette avbryt til sann
      break;  // Bryt loop
    }
  }

  while (! rfid.readCardSerial()) { // Når UID ikke blir funnet
    if ((offTime + 10000) < millis()) { // Hvis avbryt blir trykket inn
      runLoop = 0;  // Sette avbryt til sann
      break;  // Bryt loop
    }
  }

  rxNum.remove(0);

  for (int i = 0; i < 4; i++) { // card value: "xyz xyz xyz xyz xyz" (15 digits maximum; 5 pairs of xyz)hence 0<=i<=4 //
    RFID = rfid.serNum[i];
    rxNum += RFID; // store RFID value into string "rxNum" and concatinate it with each iteration
  }

  if (rxNum != rxNumOld) {
    logAccess(rxNum, 1);
  }
  rxNum.toCharArray(uidAll, 13);
}

void getUID() {  // Hente UID
  String rxNumOld = rxNum;
  rxNum.remove(0);

  for (int i = 0; i < 4; i++) //card value: "xyz xyz xyz xyz xyz" (15 digits maximum; 5 pairs of xyz)hence 0<=i<=4 //
  {
    RFID = rfid.serNum[i];
    rxNum += RFID; // store RFID value into string "rxNum" and concatinate it with each iteration
  }

  if (rxNum != rxNumOld) {
    logAccess(rxNum, 1);
  }
  rxNum.toCharArray(uidAll, 13);
}

void led(int color) {
  if (color == 3) {  // Rød LED
    digitalWrite(redLED, HIGH);
    digitalWrite(yellowLED, LOW);
    digitalWrite(greenLED, LOW);
  }
  else if (color == 2) {  // Gul LED
    digitalWrite(redLED, LOW);
    digitalWrite(yellowLED, HIGH);
    digitalWrite(greenLED, LOW);
  }
  else if (color == 1) {  // Grønn LED
    digitalWrite(redLED, LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(greenLED, HIGH);
  }
  else if (color == 0) {  // LED av
    digitalWrite(redLED, LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(greenLED, LOW);
  }
}

void getNumber() {
  ID = 0;
  char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

  offTime = millis();
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Enter ID");
  i2c_lcd.setCursor(0, 1);
  i2c_lcd.print("and press *");

  while (whichKey != '*') {
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
    if (whichKey != NO_KEY && isDigit(whichKey)) {
      ID = (ID * 10) + (whichKey - '0');
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(ID);
    }
    if (whichKey == '#') {
      runLoop = 0;
      break;
    }
  }
}

void enterUID() {
  rxNum.remove(0);
  char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

  offTime = millis();
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Enter UID");
  i2c_lcd.setCursor(0, 1);
  i2c_lcd.print("and press *");

  while (whichKey != '*') {
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
    if (whichKey != NO_KEY && isDigit(whichKey)) {
      rxNum += whichKey;
      i2c_lcd.clear();
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(rxNum);
    }
    if (whichKey == '#') {
      runLoop = 0;
      break;
    }
  }
}

void setTime() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set time/date");
  i2c_lcd.setCursor(0, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (whichKey == '*') {
      setDayRTC();

      if (runLoop == 0) {
        break;
      }

      for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
        char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey
        i2c_lcd.clear();
        i2c_lcd.setCursor(0, 0);
        i2c_lcd.print(dayRTC);
        i2c_lcd.setCursor(2, 0);
        i2c_lcd.print("/");
        i2c_lcd.setCursor(3, 0);
        i2c_lcd.print(monthRTC);
        i2c_lcd.setCursor(5, 0);
        i2c_lcd.print("/");
        i2c_lcd.setCursor(6, 0);
        i2c_lcd.print(yearRTC);
        i2c_lcd.setCursor(11, 0);
        i2c_lcd.print(hourRTC);
        i2c_lcd.setCursor(13, 0);
        i2c_lcd.print(":");
        i2c_lcd.setCursor(14, 0);
        i2c_lcd.print(minuteRTC);
        i2c_lcd.setCursor(0, 1);
        i2c_lcd.print("Press * to save");


        if (whichKey == '*') {
          clock.setYear(yearRTC);
          clock.setMonth(monthRTC);
          clock.setDate(dayRTC);
          clock.setDoW(DoWRTC);
          clock.setHour(hourRTC);
          clock.setMinute(minuteRTC);
          clock.setSecond(secondRTC);

          i2c_lcd.clear();
          i2c_lcd.setCursor(0, 0);
          i2c_lcd.print("Date and time");
          i2c_lcd.setCursor(0, 1);
          i2c_lcd.print("saved");
          offTime = millis();

          for (offTime = millis(); (offTime + 2000) > millis() && runLoop == 1;) { // Starte loop som varer i 2 sek eller til handling er ferdig
            char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

            if (whichKey == '#') {
              runLoop = 0;
              break;
            }
          }
        }

        if (whichKey == '#') {
          offTime = millis();
          runLoop = 0;
          break;
        }
      }
    }
    if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
      break;
    }
  }
}


void setDayRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set day");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      dayRTC = dayRTC + intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(dayRTC);
    }
    else if (whichKey == '*') {
      setMonthRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setMonthRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set month");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      monthRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(monthRTC);
    }
    else if (whichKey == '*') {
      setYearRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setYearRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set year");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      yearRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(yearRTC);
    }
    else if (whichKey == '*') {
      setHourRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setHourRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set hour");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      hourRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(hourRTC);
    }
    else if (whichKey == '*') {
      setMinuteRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setMinuteRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set minutes");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      minuteRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(minuteRTC);
    }
    else if (whichKey == '*') {
      setSecondRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setSecondRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set seconds");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      secondRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(secondRTC);
    }
    else if (whichKey == '*') {
      setDoWRTC();
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void setDoWRTC() {
  i2c_lcd.clear();
  i2c_lcd.setCursor(0, 0);
  i2c_lcd.print("Set day of week");
  i2c_lcd.setCursor(9, 1);
  i2c_lcd.print("Press *");
  offTime = millis();

  for (offTime = millis(); (offTime + 10000) > millis() && runLoop == 1;) { // Starte loop som varer i 10sek eller til handling er ferdig
    char whichKey = I2C_Keypad.getKey(); // Define which key is pressed with getKey

    if (isDigit(whichKey)) {
      int intWhichKey;
      intWhichKey = whichKey - '0';
      DoWRTC = intWhichKey;
      i2c_lcd.setCursor(0, 1);
      i2c_lcd.print(DoWRTC);
    }
    else if (whichKey == '*') {
      offTime = millis();
      break;
    }
    else if (whichKey == '#') {
      offTime = millis();
      runLoop = 0;
    }
  }
}

void cucaracha() {
  tone(buzzerPin, 261, times2);  //First part
  delay(times);
  tone(buzzerPin, 261, times2);
  delay(times);
  tone(buzzerPin, 261, times2);
  delay(times);
  tone(buzzerPin, 349, times2 * 2.5);
  delay(times * 1.5);
  tone(buzzerPin, 40, times2 * 2.5);
  delay(times * 2);
  tone(buzzerPin, 261, times2);
  delay(times);
  tone(buzzerPin, 261, times2);
  delay(times);
  tone(buzzerPin, 261, times2);
  delay(times);
  tone(buzzerPin, 349, times2 * 2.5);
  delay(times * 1.5);
  tone(buzzerPin, 40, times2 * 2);
  delay(times * 2);

  tone(buzzerPin, 349, times2);  //Second part
  delay(times3);
  tone(buzzerPin, 349, times2);
  delay(times3);
  tone(buzzerPin, 329, times2);
  delay(times3);
  tone(buzzerPin, 329, times2);
  delay(times3);
  tone(buzzerPin, 293, times2);
  delay(times3);
  tone(buzzerPin, 293, times2);
  delay(times3);
  tone(buzzerPin, 261, times2 * 2);
  delay(times * 3);
}

void turnOFF() {
  digitalWrite(holdekretsPin, releOFF);
}

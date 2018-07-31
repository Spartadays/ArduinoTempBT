/*  CONNECTIONS:
    A5 - SCL (LCD)
    A4 - SDA (LCD)
    2 - DHT DATA
    8 - BT TX
    9 - BT RX (use 1/2 divider!!!)
*/
#include <Wire.h>               // std Arduino library
#include <LiquidCrystal_I2C.h>  // library I2C for LCD
#include <SoftwareSerial.h>     // to BT Serial other ports than rx and tx
#include <Timers.h>             // Timers.h from Nettigo :)
#include "DHT.h"                // lib for temp and hum sensor

#define DHT11_PIN 2
#define ARX 8
#define ATX 9
#define ERROR_PIN_LED 4

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set lcd adress to 0x27
SoftwareSerial bt(ARX, ATX);    // BT(Rx & Tx from Arduino side)
DHT dht;

// Timers:
Timer timerBT;
Timer timerDHT;
Timer timerERR;

// CONST:
unsigned const short ACC_ARRAY_SIZE = 3;
unsigned const short NUM_OF_SEND_DATA = 3;

// States:
bool lcdState = true;
bool lcdLightState = true;

// To send data:
int lcdSendState = 3;           // binary states in DEC 11 => 3 etc. (lcdState|lcdLightState)
int toSendTemperature = 0;
int toSendHumidity = 0;

// Received data:
char rData[ACC_ARRAY_SIZE];
int rState = 3;

// previous var:
String prevTxt1 = "";
String prevTxt2 = "";

void lcdWrite(String row1, String row2) {
  if (prevTxt1.compareTo(row1) != 0 || prevTxt2.compareTo(row2) != 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(row1);
    lcd.setCursor(0, 1);
    lcd.print(row2);
    prevTxt1 = row1;
    prevTxt2 = row2;
  }
}

void lcdWork(String rowFirst, String rowSecond) {  // should be in loop
  if (lcdState) lcdWrite("Tempera.: " + rowFirst + " " + (char)223 + "C", "Humidity: " + rowSecond + " %RH");
  else lcd.clear();

  if (lcdLightState) lcd.backlight();
  else lcd.noBacklight();
}

void getStateBt() { // should be in loop
  if (bt.available() > 0) {
    for (int i = 0; i < ACC_ARRAY_SIZE; i++) rData[i] = bt.read();
    for (int i = 0; i < ACC_ARRAY_SIZE - 1; i++) {
      if (rData[i] == rData[i + 1]) useStateBt(rData[0]);
    }
  }
}

void useStateBt(char rCharState) {
  switch (rCharState) {
    case '3':
      lcdState = true;
      lcdLightState = true;
      rState = 3;
      break;
    case '2':
      lcdState = true;
      lcdLightState = false;
      rState = 2;
      break;
    case '1':
      lcdState = false;
      lcdLightState = true;
      rState = 1;
      break;
    case '0':
      lcdState = false;
      lcdLightState = false;
      rState = 0;
      break;
  }
  lcdSendState = rState;
}

void sendData() { // should be in loop
  if (timerBT.available()) {
    int toSendData[NUM_OF_SEND_DATA] = {int(toSendTemperature), int(toSendHumidity), int(lcdSendState)};
    for (int i = 0; i < NUM_OF_SEND_DATA; i++) {
      bt.write(toSendData[i]);
      delay(1);
    }
    timerBT.restart();
  }
}

void errorSignal(String errorTxt) {
  digitalWrite(ERROR_PIN_LED, HIGH);
  if (timerERR.available()) {
    Serial.println(errorTxt);
    lcd.clear();
    lcd.print(errorTxt);
    timerERR.restart();
  }
}

void setup() {
  dht.setup(DHT11_PIN);
  timerDHT.begin(dht.getMinimumSamplingPeriod()); // dht has its own buildin sampling period
  timerBT.begin(300);
  timerERR.begin(1000);
  Serial.begin(9600);
  lcd.begin(16, 2);
  bt.begin(9600);
  pinMode(OUTPUT, ERROR_PIN_LED);
  digitalWrite(ERROR_PIN_LED, LOW);
}

void loop() {
  getStateBt();
  if (timerDHT.available()) {
    toSendTemperature = dht.getTemperature();
    toSendHumidity = dht.getHumidity();
    timerDHT.restart();
    digitalWrite(ERROR_PIN_LED, LOW);
  }
  if (dht.getStatusString() == "OK") lcdWork(String(toSendTemperature), String(toSendHumidity));
  else errorSignal(String(dht.getStatusString()));
  sendData();
}

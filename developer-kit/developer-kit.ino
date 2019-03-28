// This is an example project.

#if defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#endif
#include "Hiber.h"

#define SERIAL_HIBER Serial2
#define SERIAL_USB Serial

// These pins are the pins on the arduino, not the modem!!!
#define PIN_WAKEUP0 31
// Deprecated
#define PIN_SELECT_RS485 33
#define PIN_WKUP_LED 35

#define PIN_LED_API_TXD 39
#define PIN_LED_API_RXD 41
#define PIN_LED_DBG_TXD 43
#define PIN_LED_DBG_RXD 45

Hiber hiber(&SERIAL_HIBER);

/*
Devboard notes:
For rev 1 (red):
- The UART connections are the same as the arduino, but it 
  requires a swap (TX to RX, instead of TX to TX)
  You are required to manually creating this swap by 
  taking 2 jump cables and connecting TXD0 to RXD2,
  and RXD0 to TXD2. Additionally, make sure that
  RXD0 and TXD0 are not going into the Arduino header,
  by bending the pins. The RXD2 and TXD2 pins are not
  used by the devboard, so you should not bend these.

  Some boards already have this fix applied.

For rev 2 (blue)
- The issue with the TX and RX swap is resolved. However, 
  if you want to use the Arduino USB serial, you have to
  wire the RXD0 (pin 0) and TXD0 (pin 1) pins to a different UART.
  You can choose TX2 (pin 16) and RX2 (pin 17). Additionally
  you'd have to make sure the SERIAL_HIBER defined above
  reflects the chosen Serial port.
  Also make sure that the RXD0 and TXD0 pins do not go into
  the Arduino header.


*/


void setup() {
  setDebugSerial(&SERIAL_USB);
  toggleDebugIO(false);

  SERIAL_HIBER.begin(19200);
  SERIAL_USB.begin(19200);

  PrintString("[API]");
  PrintString("] - Set WKUP low");
  PrintString("[ - Set WKUP high");

  // RS485 might eventually be supported on certain boards only, to reduce
  // power consumption.
  pinMode(PIN_SELECT_RS485, OUTPUT);
  pinMode(PIN_WAKEUP0, OUTPUT);
  pinMode(PIN_WKUP_LED, OUTPUT);
  
  digitalWrite(PIN_SELECT_RS485, LOW);
  digitalWrite(PIN_WAKEUP0, HIGH);
  digitalWrite(PIN_WKUP_LED, HIGH);

  // Set up the LEDs for showing RX/TX
  // Note: as we do not have a wrapper around the Serials
  // we cannot show the _actual_ TXD line.
  // For now we will show API TXD as 'api input'
  // and API RXD as 'arduino input'
  
  pinMode(PIN_LED_API_TXD, OUTPUT);
  pinMode(PIN_LED_API_RXD, OUTPUT);
  pinMode(PIN_LED_DBG_TXD, OUTPUT);
  pinMode(PIN_LED_DBG_RXD, OUTPUT);
  
  digitalWrite(PIN_LED_API_TXD, LOW);
  digitalWrite(PIN_LED_API_RXD, LOW);
  digitalWrite(PIN_LED_DBG_TXD, LOW);
  digitalWrite(PIN_LED_DBG_RXD, LOW);
  
}

// An element per pin
static unsigned long ledsLastReset[] = {0, 0, 0, 0};

int getIndexForLEDPin(int pin)
{
  switch (pin) {
    case PIN_LED_API_TXD: return 0;
    case PIN_LED_API_RXD: return 1;
    case PIN_LED_DBG_TXD: return 2;
    case PIN_LED_DBG_RXD: return 3;
    default: return -1;
  }
}

void tryTurnOffLED(int pin, unsigned long millis)
{
  int idx = getIndexForLEDPin(pin);
  if (abs(ledsLastReset[idx] - millis) < 400) return;
  ledsLastReset[idx] = millis;
  digitalWrite(pin, HIGH);
}

void turnOnLED(int pin) {
  int idx = getIndexForLEDPin(pin);
  ledsLastReset[idx] = millis();
  digitalWrite(pin, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  char x = 0;
  
  while ((x = SERIAL_HIBER.read()) != -1) SERIAL_USB.write(x);
  while ((x = SERIAL_USB.read()) != -1) {
    if (x == '[') {
      PrintString("Wakeup high\r\n");
      SetWakeupHigh(true);
    }
    else if (x == ']') {
      SetWakeupHigh(false);
      PrintString("Wakeup low\r\n");
    }
    else {
      SERIAL_HIBER.write(x);
      SERIAL_USB.write(x);
    }
  }

  tryTurnOffLED(PIN_LED_API_TXD, currentMillis);
  tryTurnOffLED(PIN_LED_API_RXD, currentMillis);
  tryTurnOffLED(PIN_LED_DBG_TXD, currentMillis);
  tryTurnOffLED(PIN_LED_DBG_RXD, currentMillis);
}

void PrintString(String str) {
  ::writeString(&SERIAL_USB, str + "\r\n");
}

void SetWakeupHigh(bool high) {
  digitalWrite(PIN_WAKEUP0, high ? HIGH : LOW);
  digitalWrite(PIN_WKUP_LED, high ? HIGH : LOW);
}

int getPinForSerial(Stream* eventSerial)
{
  if (eventSerial == &SERIAL_USB) return PIN_LED_API_RXD;
  if (eventSerial == &SERIAL_HIBER) return PIN_LED_API_TXD;
  return -1;
}

void handleSerialEvent(Stream* serial)
{
  bool on = serial->available() != 0;
  int pin = getPinForSerial(serial);
  int idx = getIndexForLEDPin(pin);
  if (pin != -1) {
    turnOnLED(pin);
    digitalWrite(pin, on ? LOW : HIGH);
  }
}

void serialEvent() {
  handleSerialEvent(&Serial);
}
void serialEvent1() {
  handleSerialEvent(&Serial1);
}
void serialEvent2() {
  handleSerialEvent(&Serial2);
}
void serialEvent3() {
  handleSerialEvent(&Serial3);
}

// This is an example project.

#include <avr/wdt.h>
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

#define STR_FAIL "Tests FAILED"
#define STR_PASS "Tests PASSED"

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
  PrintString("& - Enter Hiber LPGAN modem test cycle");

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
    else if (x == '&') {
      Test();
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

bool testBool(bool expected, String name, bool result) {
  PrintString(name + " " + String(result ? "YES" : "NO") + ", expected " + String(expected ? "YES" : "NO"));
  if (result != expected) {
    PrintString(STR_FAIL);
    // Reset chip
    wdt_enable(WDTO_30MS);
    while(1) {}
  }
  return result;
}

bool DidModemGoToSleep(Hiber::GoToSleepResult result)
{
  switch (result) {
    case Hiber::GTSR_WAKEUP0_HIGH: 
      PrintString("Did not start to sleep. Wakeup0 is high.");
      return false;
    case Hiber::GTSR_OK:
      PrintString("Sleeping works. Wakeup through pin.");
      return true;
    case Hiber::GTSR_OK_ONLY_WKUP0:
      PrintString("Sleeping works, but can only wakeup through wakeup pin.");
      return true;
    case Hiber::GTSR_ERROR:
      PrintString("Unhandled error!");
      testBool(true, "Unhandled error!", false);
      return false;
    default:
      PrintString("-- SERIOUS ERROR --");
      testBool(true, "Sleep result was odd: " + result, false);
      return false;
  }
}

void Test() {
  SetWakeupHigh(true);

  PrintString("Doing stuff");

  bool prep1 = testBool(true, "Does prepareBroadcast work?", hiber.prepareBroadcast((byte*)"testmsg", 6)) == true;

  {
    // Inside a block, so we don't fill up the stack while doing the other tests
    byte buffer[HIBER_MAX_DATA_LEN];
    for (int i = 0; i < sizeof(buffer); i++) {
      buffer[i] = i % 255;
    }

    prep1 = testBool(true, "Does a big prepareBroadcast work?", hiber.prepareBroadcast(buffer, sizeof(buffer))) == true;
  }

  String iso8601DateTime;
  bool gdt1 = testBool(true, "Does getDateTime work?", hiber.getDateTime(&iso8601DateTime)) == true;
  PrintString("Resulted datetime: " + iso8601DateTime);

  // Wakeup pin is high, so check for that
  Hiber::GoToSleepResult sleepResult;
  hiber.goToSleep(&sleepResult, nullptr, nullptr);

  bool sleep1 = !DidModemGoToSleep(sleepResult) && sleepResult == Hiber::GTSR_WAKEUP0_HIGH;

  bool step1_correct = prep1 && sleep1 && gdt1;
  testBool(true, "Step 1 correct?", step1_correct);
  

  // Step 2: set_gps_mode should toggle set_datetime and set_location
  bool gps1 = testBool(true, "Does set_gps_mode(false) work?", hiber.setGPSMode(false)) == true;

  bool sdt1 = testBool(true, "Does set_datetime('2017-05-01T16:30:22Z') work?", hiber.setDateTime("2017-05-01T16:30:22Z")) == true;
  bool sl1 = testBool(true, "Does set_location() work?", hiber.setLocation(51.1234, 5.2133, 0)) == true;
  bool rn1 = testBool(true, "Does run_nmea() work?", hiber.sendNMEA("$GPGSV,4,3,15,14,35,307,,22,35,012,,12,26,199,,18,21,240,*72\r\n")) == true;

  bool step2_correct = gps1 && sdt1 && sl1 && rn1;
  testBool(true, "Step 2 correct?", step2_correct);
  
  
  // Step 3: Enable GPS mode, set_datetime and set_location should fail
  
  bool gps2 = testBool(true, "Does set_gps_mode(true) work?", hiber.setGPSMode(true));

  bool sdt2 = testBool(false, "Does set_datetime('2017-05-01T16:30:22Z') work?", hiber.setDateTime("2017-05-01T16:30:22Z")) == false;
  bool sl2 = testBool(false, "Does set_location() work?", hiber.setLocation(51.1234, 5.2133, 0)) == false;
  bool rn2 = testBool(false, "Does run_nmea() work?", hiber.sendNMEA("$GPGSV,4,3,15,14,35,307,,22,35,012,,12,26,199,,18,21,240,*72\r\n")) == false;

  bool step3_correct = gps2 && sdt2 && sl2 && rn2;
  testBool(true, "Step 3 correct?", step3_correct);

  // Step 4: Check sleep mode
  
  SetWakeupHigh(false);
  hiber.goToSleep(&sleepResult, nullptr, nullptr);

  bool sleep2 = DidModemGoToSleep(sleepResult) && sleepResult == Hiber::GTSR_OK;
  
  PrintString("Waking up...");

  SetWakeupHigh(true);
  for (int i = 0; i < 3; i++) {
    String startupString = readln(&SERIAL_HIBER);
    PrintString("Response: " + startupString);
    String tmp[0];
    int arg_count;
    if (hiber.parseResponse(startupString, tmp, 0, &arg_count) == Hiber::INVALID_RESPONSE_DEVICE_JUST_BOOTED) {
      break;
    }
  }

  bool gps3 = testBool(true, "Does set_gps_mode(false) work?", hiber.setGPSMode(false));
  
  SetWakeupHigh(false);
  hiber.goToSleep(&sleepResult, nullptr, nullptr);

  bool sleep3 = DidModemGoToSleep(sleepResult) && sleepResult == Hiber::GTSR_OK_ONLY_WKUP0;

  bool step4_correct = sleep2 && gps3 && sleep3;
  testBool(true, "Step 4 correct?", step4_correct);

  if (step1_correct && step2_correct && step3_correct && step4_correct) {
    PrintString(STR_PASS);
  }
  else {
    PrintString(STR_FAIL);
  }
  
  // Boot the chip again
  SetWakeupHigh(true);
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

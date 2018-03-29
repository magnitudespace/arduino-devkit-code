#include <avr/wdt.h>
#include "Hiber.h"

#define SERIAL_HIBER Serial2
#define SERIAL_USB Serial

#define PIN_WAKEUP0 31
#define PIN_SELECT_RS485 33
#define PIN_WKUP_LED 35

#define STR_FAIL "Tests FAILED"
#define STR_PASS "Tests PASSED"

Hiber hiber(&SERIAL_HIBER);

void setup() {
  SERIAL_HIBER.begin(19200);
  SERIAL_USB.begin(19200);
  SERIAL_USB.write("[API]\n");
  SERIAL_USB.write("] - Set WKUP low\n");
  SERIAL_USB.write("[ - Set WKUP high\n");
  SERIAL_USB.write("& - Enter Hiber LPGAN modem test mode\n");

  // RS485 might eventually be supported on certain boards only, to reduce
  // power consumption.
  pinMode(PIN_SELECT_RS485, OUTPUT);
  pinMode(PIN_WAKEUP0, OUTPUT);
  pinMode(PIN_WKUP_LED, OUTPUT);
  digitalWrite(PIN_SELECT_RS485, LOW);
  digitalWrite(PIN_WAKEUP0, HIGH);
  digitalWrite(PIN_WKUP_LED, HIGH);
}

String readln(Stream *stream);

void loop() {
  char x = 0;
  while ((x = SERIAL_HIBER.read()) != -1) SERIAL_USB.write(x);
  while ((x = SERIAL_USB.read()) != -1) {
    if (x == '[') {
      SERIAL_USB.write("Wakeup high\r\n");
      SetWakeupHigh(true);
    }
    else if (x == ']') {
      SetWakeupHigh(false);
      SERIAL_USB.write("Wakeup low\r\n");
    }
    else if (x == '&') {
      Test();
    }
    else {
      SERIAL_HIBER.write(x);
      SERIAL_USB.write(x);
    }
  }
}

void DebugString(String str) {
  writeString(&SERIAL_USB, str + "\r\n");
}

void SetWakeupHigh(bool high) {
  digitalWrite(PIN_WAKEUP0, high ? HIGH : LOW);
  digitalWrite(PIN_WKUP_LED, high ? HIGH : LOW);
}

bool testBool(bool expected, String name, bool result) {
  DebugString(name + " " + String(result ? "YES" : "NO") + " == " + String(expected ? "YES" : "NO"));
  if (result != expected) {
    DebugString(STR_FAIL);
	// Reset chip
    wdt_enable(WDTO_30MS);
    while(1) {};
  }
  return result;
}

void Test() {
  SetWakeupHigh(true);

  DebugString("Waiting for newline");
  readln(&SERIAL_USB);

  DebugString("Doing stuff");


  bool prep1 = testBool(true, "Does prepareBroadcast work?", hiber.prepareBroadcast((byte*)"testmsg", 6)) == true;

  String iso8601DateTime;
  bool gdt1 = testBool(true, "Does getDateTime work?", hiber.getDateTime(&iso8601DateTime)) == true;
  DebugString(" (" + iso8601DateTime + ")");

  Hiber::GoToSleepResult sleepResult;
  hiber.goToSleep(&sleepResult, nullptr, nullptr);

  bool sleep1;
  switch (sleepResult){
    case Hiber::GTSR_WAKEUP0_HIGH: 
      DebugString("Did not start to sleep. Wakeup0 is high.");
      sleep1 = true;
      break;
    case Hiber::GTSR_OK:
      DebugString("Sleeping works. Wakeup through pin.");
      sleep1 = false;
      break;
    case Hiber::GTSR_OK_ONLY_WKUP0:
      DebugString("Sleeping works, but can only wakeup through wakeup pin.");
      sleep1 = false;
      break;
    case Hiber::GTSR_ERROR:
      DebugString("Unhandled error!");
      sleep1 = false;
      break;
    default:
      DebugString("-- SERIOUS ERROR --");
      sleep1 = false;
      return;
  }
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

  bool sleep2;
  switch (sleepResult){
    case Hiber::GTSR_WAKEUP0_HIGH: 
      DebugString("Did not start to sleep. Wakeup0 is high.");
      sleep2 = false;
      break;
    case Hiber::GTSR_OK:
      DebugString("Sleeping works. Wakeup through pin.");
      sleep2 = true;
      break;
    case Hiber::GTSR_OK_ONLY_WKUP0:
      DebugString("Sleeping works, but can only wakeup through wakeup pin.");
      sleep2 = false;
      break;
    case Hiber::GTSR_ERROR:
      DebugString("Unhandled error!");
      sleep2 = false;
      break;
    default:
      DebugString("-- SERIOUS ERROR --");
      sleep2 = false;
      return;
  }

  SetWakeupHigh(true);
  readln(&SERIAL_USB);

  bool gps3 = testBool(true, "Does set_gps_mode(false) work?", hiber.setGPSMode(false));
  
  SetWakeupHigh(false);
  hiber.goToSleep(&sleepResult, nullptr, nullptr);

  bool sleep3;
  switch (sleepResult){
    case Hiber::GTSR_WAKEUP0_HIGH: 
      DebugString("Did not start to sleep. Wakeup0 is high.");
      sleep3 = false;
      break;
    case Hiber::GTSR_OK:
      DebugString("Sleeping works. Wakeup through pin.");
      sleep3 = false;
      break;
    case Hiber::GTSR_OK_ONLY_WKUP0:
      DebugString("Sleeping works, but can only wakeup through wakeup pin.");
      sleep3 = true;
      break;
    case Hiber::GTSR_ERROR:
      DebugString("Unhandled error!");
      sleep3 = false;
      break;
    default:
      DebugString("-- SERIOUS ERROR --");
      sleep3 = false;
      return;
  }

  bool step4_correct = sleep2 && gps3 && sleep3;
  testBool(true, "Step 4 correct?", step4_correct);

  if (step1_correct && step2_correct && step3_correct && step4_correct) {
    DebugString(STR_PASS);
  }
  else {
    DebugString(STR_FAIL);
  }
  
  // Boot the chip again
  SetWakeupHigh(true);
}



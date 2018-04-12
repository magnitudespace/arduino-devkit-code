#pragma once

#include <Arduino.h>

// This is the maximum amount of bytes you 
// send through the modem
#define HIBER_MAX_DATA_LEN (144)

class Hiber {
public:
  typedef enum : byte {
    CT_INFO = 0,
    CT_ERROR = 1,
    CT_DEBUG = 2,
  } CodeTypes;

  typedef enum : byte {
    CC_COMMAND_PARSING = 1,
    CC_STRING_INTERPOLATION = 2,
    CC_ARGUMENT_CONVERSION = 3,
    CC_COMMAND_HANDLING = 4,
    CC_RAW_INPUT_HANDLING = 5,
    CC_COMMAND_EXECUTION = 6,
  } CodeCategories;

  typedef enum : short {
    RESPONSE_OK = 600,
    RESPONSE_STARTING_TO_SLEEP = 602,
    RESPONSE_CANNOT_SLEEP_WKUP0_HIGH = 603,
    RESPONSE_ERROR_GPS_IS_ENABLED = 633,

    // Arduino response processing error codes
    INVALID_RESPONSE_TOO_SHORT = 950, // Unable to parse response (too short to be valid)
    INVALID_RESPONSE_NO_END = 951, // Unable to parse response, because it had no termination character
    INVALID_RESPONSE_STRING_ARG = 952, // Unable to parse a string in the response
    INVALID_RESPONSE_NO_BEGIN = 953, // Unable to parse response (did not start with API( )
    INVALID_RESPONSE_DEVICE_JUST_BOOTED = 954, // Received booting message
  } Codes;

  typedef enum : byte {
    GTSR_OK = 0,
    GTSR_OK_ONLY_WKUP0 = 1,
    GTSR_ERROR = 10,
    GTSR_WAKEUP0_HIGH = 11,
  } GoToSleepResult;

  Hiber(Stream *serial_port);

  // Actual commands. You can find the declarations inside HiberCalls.cpp
  bool prepareBroadcast(byte data[], int data_len);
  bool sendNMEA(String nmea_command);
  bool getNextWakeupTime(int *reason, int *seconds_left);
  bool getDateTime(String *iso8601DateTime);
  bool setDateTime(String iso8601DateTime);
  bool setGPSMode(bool enabled);
  bool setLocation(float latitude, float longitude, float altitude);
  bool doGPSFix();
  bool goToSleep(GoToSleepResult *result, int *reason, int *seconds_left);

  // Functionality for building commands
  void sendCommandName(String command);
  void sendCommandFinish();

  void sendArgumentString(String value);
  void sendArgumentInt(int value, bool hex = false);
  void sendArgumentBool(bool value);
  void sendArgumentFloat(float value);

  // Only returns the response code. If you still need 
  // the response text (unparsed), call getResponseText()
  short readResponse() {
    String argv[0];
    int argc;
    return readResponse(argv, 0, &argc);
  };

  // Returns the response code
  short readResponse(String arguments[], int max_argument_count, int *argument_count);
  
  short parseResponse(String response, String arguments[], int max_argument_count, int *argument_count);

  
  static CodeCategories getCodeCategory(short code) { return (CodeCategories)(code / 100); };
  static CodeTypes getCodeType(short code) { return (CodeTypes)((code % 100) / 25); };

  static bool isCodeInfo(int code) { return getCodeType(code) == CT_INFO; };
  static bool isCodeError(int code) { return getCodeType(code) == CT_ERROR; };
  static bool isCodeDebug(int code) { return getCodeType(code) == CT_DEBUG; };

  String getResponseText() const { return previous_response; };
private:
  void sendArgumentStart();
  void writeString(String value);

  Stream *serial_port;
  bool command_with_arguments;
  String previous_response;
  short previous_response_code;
  String last_command;
  
};

void writeString(Stream *stream, String value);
String readln(Stream *stream);


// Debug IO between the Arduino and the Hiber modem
void setDebugSerial(Stream* stream);
void toggleDebugIO(bool enable);
  

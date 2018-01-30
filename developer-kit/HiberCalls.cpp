#include "Hiber.h"


bool Hiber::prepareBroadcast(byte data[], int data_len)
{
  sendCommandName("set_payload");
  sendArgumentInt(data_len);
  sendCommandFinish();

  // Wait for OK
  int code = readResponse();

  if (code == RESPONSE_OK) {
    // delay(00);
    // Write the data...
    //Serial.write("> ", 2);
    //Serial.write(data, data_len);
    //Serial.write("\r\n", 2);
    for (int i = 0; i < data_len; i++) {
      delay(100);
      serial_port->write(data[i]);
    }
    //serial_port->write(data, data_len);
    serial_port->flush();
    
    code = readResponse();
  }

  if (code != RESPONSE_OK) {
    
      return false;
  }
  return true;
}

bool Hiber::sendNMEA(String nmea_command)
{
  sendCommandName("run_nmea");
  sendArgumentString(nmea_command);
  sendCommandFinish();
  
  int code = readResponse();
  return code == RESPONSE_OK;
}

bool Hiber::getNextWakeupTime(int *reason, int *seconds_left)
{
  sendCommandName("get_next_wakeup_time");
  sendCommandFinish();
  
  String arguments[2];
  int arguments_returned = 0;
  short code = readResponse(arguments, 2, &arguments_returned);

  if (code != RESPONSE_OK ||
      arguments_returned != 2) return false;

  *reason = arguments[0].toInt();
  *seconds_left = arguments[1].toInt();

  return true;
}

bool Hiber::getDateTime(String *iso8601DateTime)
{
  sendCommandName("get_datetime");
  sendCommandFinish();

  String arguments[1];
  int arguments_returned = 0;
  short code = readResponse(arguments, 1, &arguments_returned);

  if (code != RESPONSE_OK ||
      arguments_returned != 1) return false;

  *iso8601DateTime = arguments[0];

  return true;
}

bool Hiber::setDateTime(String iso8601DateTime)
{
  sendCommandName("set_datetime");
  sendArgumentString(iso8601DateTime);
  sendCommandFinish();

  String arguments[1];
  int arguments_returned = 0;
  short code = readResponse(arguments, 1, &arguments_returned);

  if (code != RESPONSE_OK ||
      arguments_returned != 1) return false;
  return true;
}

bool Hiber::setGPSMode(bool enabled)
{
  sendCommandName("set_gps_mode");
  sendArgumentBool(enabled);
  sendCommandFinish();
  
  String arguments[1];
  int arguments_returned = 0;
  short code = readResponse(arguments, 1, &arguments_returned);

  if (code != RESPONSE_OK ||
      arguments_returned != 1) return false;
  return true;
}

bool Hiber::setLocation(float latitude, float longitude, float altitude)
{
  sendCommandName("set_location");
  sendArgumentFloat(latitude);
  sendArgumentFloat(longitude);
  sendArgumentFloat(altitude);
  sendCommandFinish();
  
  String arguments[2]; // Lat and lon
  int arguments_returned = 0;
  short code = readResponse(arguments, 2, &arguments_returned);

  if (code != RESPONSE_OK ||
      arguments_returned != 2) return false;
  return true;
}

bool Hiber::doGPSFix()
{
  sendCommandName("do_gps_fix");
  sendCommandFinish();

  return readResponse() == RESPONSE_OK;
}

bool Hiber::goToSleep(GoToSleepResult *result, int *reason, int *seconds_left)
{
  sendCommandName("go_to_sleep");
  sendCommandFinish();

  String arguments[2];
  int arguments_returned = 0;
  short code = readResponse(arguments, 2, &arguments_returned);

  switch (code) {
    case RESPONSE_STARTING_TO_SLEEP: {
      int _reason = arguments[0].toInt();
      if (_reason == 0) {
        *result = GTSR_OK_ONLY_WKUP0;
      }
      else {
        *result = GTSR_OK;
      }
      if (reason != nullptr) *reason = arguments[0].toInt();
      if (reason != nullptr) *seconds_left = arguments[1].toInt();
      return true;
    }
    case RESPONSE_CANNOT_SLEEP_WKUP0_HIGH: {
      *result = GTSR_WAKEUP0_HIGH;
      return false;
    }
    default: {
      *result = GTSR_ERROR;
      return false;
    }
  }
}



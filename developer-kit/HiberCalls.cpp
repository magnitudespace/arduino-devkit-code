// This file includes some functionality of the Hiber modem API,
// that can be used as an example when implementing additional
// API calls.

#include "Hiber.h"

// Upload a payload to the modem, to be transmitted to
// the Hiber satellite in the next satellite pass.
bool Hiber::prepareBroadcast(byte data[], int data_len)
{
  sendCommandName("set_payload");
  sendArgumentInt(data_len);
  sendCommandFinish();

  // Wait for OK
  int code = readResponse();

  if (code == RESPONSE_OK) {
    // Now, we should write the amount of bytes that we said
    // we would write...
    serial_port->write(data, data_len);
    serial_port->flush();
    
    code = readResponse();
  }

  if (code != RESPONSE_OK) {
      return false;
  }
  return true;
}

// Send an NMEA 0183 string to the modem
// Returns true if the content was valid
bool Hiber::sendNMEA(String nmea_command)
{
  sendCommandName("run_nmea");
  sendArgumentString(nmea_command);
  sendCommandFinish();
  
  int code = readResponse();
  return code == RESPONSE_OK;
}

// Request next wakeup 'reason' (and seconds left) from the modem
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

// Request the date and time that the modem currently holds
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

// Change the date and time of the modem
// Might not be available when the GPS is enabled.
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

// Enable or disable the GPS. Certain functionality
// is not available when the GPS is enabled.
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

// Set the modem location in the world.
// The more precise these values are, the better the reception will be.
// Might not be available when the GPS is enabled.
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

// Requests a GPS fix on the modem. Will enqueue the request
// and handle it in the background. You can put the wakeup pin
// low and the modem will automatically go back to sleep when
// either a fix or a timeout occurred.
bool Hiber::doGPSFix()
{
  sendCommandName("do_gps_fix");
  sendCommandFinish();

  return readResponse() == RESPONSE_OK;
}

// Try to make the modem go to sleep.
// It is recommended to call this after the modem has been 
// woken up through the wakeup pin.
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



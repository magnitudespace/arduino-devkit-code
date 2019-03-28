// This file includes some functionality of the Hiber modem API,
// that can be used as an example when implementing additional
// API calls.

#include "Hiber.h"

// ********Being deprecated in newer modems********
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

// ********Beging deprecated in newer modems********
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

// Returns information about the current modem firmware
bool Hiber::getFirmwareVersion(String *firmware_version)
{
    sendCommandName("get_firmware_version");
    sendCommandFinish();

    String arguments[1];
    int arguments_returned = 0;
    short code = readResponse(arguments, 1, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 1) return false;

    *firmware_version = arguments[0];

    return true;
}

// Returns detailed modem information
bool Hiber::getModemInfo(String *HW_type_str, int *HW_type_int, int *FW_version, String *modem_no_str, int *modem_no_int)
{
    sendCommandName("get_modem_info");
    sendCommandFinish();

    String arguments[5];
    int arguments_returned = 0;
    short code = readResponse(arguments, 5, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 5) return false;

    *HW_type_str = arguments[0];
    *HW_type_int = arguments[1].toInt();
    *FW_version = arguments[2].toInt();
    *modem_no_str = arguments[3];
    *modem_no_int = arguments[4].toInt();

    return true;
}

// Sets the modem number as a pair of 2x4 hex digits
// e.g. "ABCD 1234". Only available from firmware 1.x 
bool Hiber::setModemNumber(String modem_number)
{
    sendCommandName("set_modem_number");
    sendArgumentString(modem_number);
    sendCommandFinish();

    return readResponse() == RESPONSE_OK;
}

// Returns the current location of the modem in the world
// Latitude & Longitude are returned in decimal degrees format
// Time since last GPS fix and time to next fix are also returned
bool Hiber::getLocation(float *latitude, float *longitude, long *seconds_since_last_fix, long *seconds_to_next_fix, float *altitude)
{
    sendCommandName("get_location");
    sendCommandFinish();

    String arguments[5];
    int arguments_returned = 0;
    short code = readResponse(arguments, 5, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 5) return false;

    *latitude = arguments[0].toFloat();
    *longitude = arguments[1].toFloat();
    *seconds_since_last_fix = arguments[2].toInt();
    *seconds_to_next_fix = arguments[3].toInt();
    *altitude = arguments[4].toFloat();

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

    String arguments[5]; 
    int arguments_returned = 0;
    short code = readResponse(arguments, 5, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 5) return false;
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

// Upload a payload to the modem, to be transmitted to
// the Hiber satellite in the next satellite pass.
bool Hiber::setPayload(byte data[], int data_len)
{
    sendCommandName("set_payload");
    sendArgumentInt(data_len);
    sendCommandFinish();

    // Wait for OK
    short code = readResponse();
    
    // This delay is required for the faker
    delay(500);
    
    String arguments[1];
    int arguments_returned = 0;

    if (code == RESPONSE_OK) {
        // Now, we should write the amount of bytes that we said
        // we would write...
        serial_port->write(data, data_len);
        serial_port->flush();
                
        code = readResponse(arguments, 1, &arguments_returned);
    }

    if (code != RESPONSE_OK ||
        arguments_returned != 1 ||
        arguments[0].toInt() != data_len) return false;

    return true;
}

// Request next wakeup alarm and 'reason' (and seconds left) from the modem
bool Hiber::getNextAlarm(int *reason, int *seconds_left)
{
    sendCommandName("get_next_alarm");
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

// Returns the number of seconds until the next expected
// satellite overhead pass
bool Hiber::getNextPass(long *seconds_left)
{
    sendCommandName("get_next_pass");
    sendCommandFinish();

    String arguments[1];
    int arguments_returned = 0;
    short code = readResponse(arguments, 1, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 1) return false;

    *seconds_left = arguments[0].toInt();

    return true;
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

// Enables the debug required for using the Hiber-Faker
// Warning, antenna must be attached if set to False
// otherwise the RF circuitry may be damaged
bool Hiber::togglePayloadOverDebug(bool enabled)
{
    sendCommandName("toggle_payload_over_debug");
    sendArgumentBool(enabled);
    sendCommandFinish();

    String arguments[1];
    int arguments_returned = 0;
    short code = readResponse(arguments, 1, &arguments_returned);

    if (code != RESPONSE_OK ||
        arguments_returned != 1) return false;
    return true;
}

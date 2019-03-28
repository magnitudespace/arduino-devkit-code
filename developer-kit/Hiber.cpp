// This file mainly handles the Hiber modem API input
// and output. The argument building code should provide
// a save way for the user to send content, as it
// automatically escapes characters that would
// otherwise be invalid.

#include "Hiber.h"

static bool debug_io = false;
static Stream* debug_port = NULL;

void setDebugSerial(Stream* port)
{
    debug_port = port;
}

void toggleDebugIO(bool enabled)
{
    debug_io = enabled;
}

String getHex(char x)
{
    String ret = String(x, HEX);
    if (x < 0x10) ret = "0" + ret;
    return ret;
}

void flushInput(Stream *stream, Stream *debug)
{
    while (stream->available()) {
        byte c = stream->read();
        if (debug_io) {
            debug->write(c);
        }
    }
}

String readln(Stream *stream)
{
    String tmp = "";
    tmp.reserve(20);
    char prev = 0;
    unsigned long start_time = millis();

    if (debug_io) ::writeString(debug_port, "< ");

    while (true) {
        if (!stream->available()) {
            // Detect timeout
            if ((millis() - start_time) >= 7000) {
                ::writeString(debug_port, "timeout\r\n");
                return tmp;
            }
            continue;
        }
        else {
            char cur = stream->read();
            if (debug_io) {
                //debug_port->print(getHex(cur));
            }

            if (prev == '\r') {
                if (cur == '\n') {
                    return tmp;
                }
            }
            else if (cur != '\r') {
                tmp += cur;
            }

            prev = cur;
        }
    }

    return tmp;
}

// This function will write a String to a Stream,
// byte-for-byte, and then flushes it.
void writeString(Stream *stream, String value)
{
    if (stream == nullptr) {
        // Lets not try to access a null pointer
        return;
    }
    stream->print(value);
    stream->flush();
}

void Hiber::writeString(String value)
{
    if (debug_io) ::writeString(debug_port, "> " + value);
    ::writeString(serial_port, value);
}

Hiber::Hiber(Stream *serial_port)
{
    this->serial_port = serial_port;
    this->command_with_arguments = false;
}

// This function replaces the current command buffer
// with 'command' as its only contents
void Hiber::sendCommandName(String command)
{
    last_command = command;
}

// This function will either add '(' or ',', depending
// on whether an argument was already started
void Hiber::sendArgumentStart()
{
    if (!command_with_arguments) {
        command_with_arguments = true;
        last_command += '(';
    }
    else {
        last_command += ',';
    }
}

void Hiber::sendCommandFinish()
{
    if (command_with_arguments) {
        last_command += ')';
    }

    last_command += "\r\n";

    flushInput(serial_port, debug_port);
    writeString(last_command);
    command_with_arguments = false;
}

void Hiber::sendArgumentString(String value)
{
    sendArgumentStart();

    last_command += '"';

    int length = value.length();
    for (int i = 0; i < length; i++) {
        char c = value[i];

        // Handle raw characters
        if (c == '\r') {
            last_command += "\\r";
            continue;
        }
        if (c == '\n') {
            last_command += "\\n";
            continue;
        }

        // Escape special characters
        if (c == '\\' || c == '"' || c == '\'') {
            last_command += '\\';
        }

        last_command += c;
    }

    last_command += '"';
}

void Hiber::sendArgumentInt(int value, bool hex)
{
    sendArgumentStart();
    String characters = String(value, hex ? HEX : DEC);

    if (hex) {
        last_command += "0x";
    }

    last_command += characters;
}

void Hiber::sendArgumentBool(bool value)
{
    sendArgumentStart();
    last_command += (value == true ? "true" : "false");
}

void Hiber::sendArgumentFloat(float value)
{
    String characters = String(value, 5); // decimals as second arg
    // Floats are strings ("123.456")
    sendArgumentString(characters);
}

static String parseStringArgument(String str, int *nextOffset, bool *valid)
{
    if (str[0] != '"') {
        // Ehm...
        *nextOffset = -1;
        *valid = false;
        return "";
    }

    int nextQuote;
    int nextQuoteMinOffset = 1;

    do {
        nextQuote = str.indexOf('"', nextQuoteMinOffset);

        if (nextQuote == -1) {
            // Invalid contents
            *nextOffset = -1;
            *valid = false;
            return "";
        }

        if (nextQuote > 1) {
            // See if previous value is a slash
            if (str[nextQuote - 1] == '\\') {
                // Ignore.
                nextQuoteMinOffset = nextQuote + 1;
                continue;
            }

            // Oh? Not a slash? We should be at the end, then.
        }


        if (str[nextQuote + 1] == ')') {
            // Yes, pretty sure we are at the end.
            // Make sure we communicate this back
            *nextOffset = -1;
        }
        else if (str[nextQuote + 1] == ';') {
            // Okay, more stuff to handle
            *nextOffset = nextQuote + 1;
        }

        *valid = true;
        // Return the string
        return str.substring(1, nextQuote - 2);
    } while (true);

}

short Hiber::parseResponse(String response, String arguments[], int max_argument_count, int *argument_count)
{
    if (response.length() < 7) {
        return INVALID_RESPONSE_TOO_SHORT;
    }

    if (debug_io) ::writeString(debug_port, "+> " + response);

    // Just make sure we begin at a valid API(, in case of
    // random stuff in front of it.
    int apiStartIndexOf = response.indexOf("API(");
    if (apiStartIndexOf != -1) {
        response = response.substring(apiStartIndexOf);
    }

    if (debug_io) ::writeString(debug_port, " ..   " + response + " <+\r\n");

    if (response.startsWith("API(")) {
        String codeStr = response.substring(4, 7);
        short code = (short)codeStr.toInt();
        int arg_count = 0;

        if (response[7] == ':' && max_argument_count > 0) {
            // We've got a reponse with arguments: API(123: foo; 1; "bar")
            String args = response.substring(3 + 1 + 3 + 2); // skip API(123:

            bool stop = false;
            for (int i = 0; i < max_argument_count; i++) {
                // Check if we've got an argument delimiter
                int offsetArgTerminator = args.indexOf(';');
                if (offsetArgTerminator == -1) {
                    // Apparently not, check for end-of-arguments character
                    offsetArgTerminator = args.indexOf(')');
                    if (offsetArgTerminator == -1) {
                        // Odd, only received half of the command
                        return INVALID_RESPONSE_NO_END;
                    }
                    // Signal that we have only 1 left
                    stop = true;
                }

                if (args[0] == '"') {
                    if (debug_io) ::writeString(debug_port, "Reading string\r\n");
                    int nextOffset = 0;
                    bool valid = false;
                    String argument = parseStringArgument(args, &nextOffset, &valid);
                    if (!valid) {
                        return INVALID_RESPONSE_STRING_ARG;
                    }

                    arguments[i] = argument;
                    arg_count++;
                    if (nextOffset == -1) stop = true;
                    else offsetArgTerminator = nextOffset;
                }
                else {
                    // Read until the offsetArgTerminator
                    arguments[i] = args.substring(0, offsetArgTerminator);
                    arg_count++;
                }


                if (stop) break;
                args = args.substring(offsetArgTerminator + 2);
            }

        }

        *argument_count = arg_count;

        return code;
    }
    else if (response.startsWith("Hiber API (")) {
        char commitHash[50];

        int results = sscanf(response.c_str(), "Hiber API (Build %s @", &commitHash);

        if (results == 1) {
            // Okay, commit hash loaded
            String rest = response.substring(response.indexOf("@") + 2);
            String buildDate = rest.substring(0, rest.indexOf(')'));

            ::writeString(debug_port, "Booted, " + String(commitHash) + " build at " + buildDate + "\r\n");

            return INVALID_RESPONSE_DEVICE_JUST_BOOTED;
        }
        else {
            if (debug_io) ::writeString(debug_port, "Error\r\n");
        }
    }
    return INVALID_RESPONSE_NO_BEGIN;
}

// Read and parse a response from the Hiber modem
short Hiber::readResponse(String arguments[], int max_argument_count, int *argument_count)
{
    String str = readln(serial_port);
    previous_response = str;

    if (debug_io) ::writeString(debug_port, "Input: " + previous_response + "\r\n");

    short code = parseResponse(previous_response, arguments, max_argument_count, argument_count);

    if (code == INVALID_RESPONSE_DEVICE_JUST_BOOTED && last_command.length() != 0) {
        if (debug_io) ::writeString(debug_port, "Trying to read next line...\r\n");
        
        str = readln(serial_port);
        previous_response = str;

        code = parseResponse(previous_response, arguments, max_argument_count, argument_count);

        if (code != RESPONSE_OK) {
            if (debug_io) ::writeString(debug_port, "Retrying command...\r\n");
            writeString(last_command);

            str = readln(serial_port);
            previous_response = str;

            code = parseResponse(previous_response, arguments, max_argument_count, argument_count);
        }
    }

    previous_response_code = code;
    return code;
}

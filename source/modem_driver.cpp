// NB-IoT modem driver for NB-IoT example application

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "example-mbedos-blinky/utilities.h"
#include "example-mbedos-blinky/serial_driver.h"
#include "example-mbedos-blinky/modem_driver.h"

#define AT_TERMINATOR        "\r\n"
#define AT_RX_POLL_TIMER_MS  100
#define AT_OK                "OK\r\n"
#define AT_ERROR             "ERROR\r\n"

Timer timer;




bool Nbiot::sendPrintf(const char * pFormat, ...)
{
    va_list args;
    bool success = false;
    uint32_t len = 0;

    if (gInitialised)
    {
    	char gTxBuf [MAX_LEN_SEND_STRING];

        va_start(args, pFormat);
        len = vsnprintf(gTxBuf, sizeof(gTxBuf), pFormat, args);
        va_end(args);

        printf("TX: %s \r\n", gTxBuf);
        success = gpSerialPort->transmitBuffer((const char *) gTxBuf);
    }

    return success;
}


// (2)
// Check the modem interface for received characters, copying each one into
// pBuf.  If an AT_TERMINATOR is found, or lenBuf characters have been
// received, return the number of characters (including the AT_TERMINATOR),
// otherwise return 0.  No NULL terminator is added to pBuf.
// Get characters (up to lenBuf of them) from the NB-IoT module into pBuf.
// If an AT terminator is found, or lenBuf characters have been read,
// return a count of the number of characters (including the AT terminator),
// otherwise return 0.

uint32_t Nbiot::getLine(char * pBuf, uint32_t lenBuf)
{
    int32_t x;
    uint32_t returnLen = 0;

    if (gInitialised){
        if (gLenRx < lenBuf){
            do{
                x = gpSerialPort->receiveChar();
                if (x >= 0)
                {
                    *(pBuf + gLenRx) = (char) x;
                    gLenRx++;
                    if (x == AT_TERMINATOR[gMatched]){
                        gMatched++;
                        //printf("144 %d %d %d %d %d\r\n", gLenRx, lenBuf, (int)x, (int)gMatched, sizeof(AT_TERMINATOR) - 1);
                    }
                    else{
                        gMatched = 0;
                    }
                }
            } while ((x >= 0) && (gLenRx < lenBuf) && (gMatched < sizeof(AT_TERMINATOR) - 1));
        }
        
        if ((gMatched == sizeof(AT_TERMINATOR) - 1) || (gLenRx == lenBuf)) // -1 to omit 0 of string
        {
            // A terminator has been found, or we've hit a buffer limit, so let the caller know to
            // handle the line and reset the tracking variables for next time
            returnLen = gLenRx;
            gLenRx = 0;
            gMatched = 0;
        }    
    }
    return returnLen;
}



// (3)
// Tick along the process of receiving characters from the modem AT interface.
// Callback to handle AT stuff received from the NBIoT module
void Nbiot::rxTick()
{
    uint32_t len = getLine (gRxBuf, sizeof (gRxBuf));

    if (len > sizeof(AT_TERMINATOR) - 1) // -1 to omit NULL terminator
    {
        printf ("RX %d: \"%.*s\".\r\n", (int) (len - (sizeof(AT_TERMINATOR) - 1)), (int) (len - (sizeof(AT_TERMINATOR) - 1)), gRxBuf);
        if (gpResponse == NULL)
        {
            gLenResponse = len;
            gpResponse = gRxBuf;
        }
    }
}


// (4)
// Wait for a response from the modem, used during transmit operations.
// If pExpected is not NULL, AtResponse will indicate if the received string
// starts with the characters at pExpected (which must be a NULL terminated
// string), otherwise it will indicate if the standard strings "OK" and
// "ERROR" have been received, otherwise it will indicate that something
// other has been received. Up to responseBufLen received characters are
// stored at pResponseBuf and a NULL terminator is added.  The number of
// includes the AT terminator (AT_TERMINATOR).  Any characters over responseBufLen
// are discarded.
// Wait for an AT response.  If pExpected is not NULL and the
// AT response string begins with this string then say so, else
// wait for the standard "OK" or "ERROR" responses for a little
// while, else time out. The response string is copied into
// gpResponse if it is non-NULL (and a null terminator is added).

Nbiot::AtResponse Nbiot::waitResponse(
		const char  *pExpected        /*NULL*/,
		time_t       timeoutSeconds   /*DEFAULT_RESPONSE_TIMEOUT_SECONDS*/,
		char        *pResponseBuf     /*NULL*/,
		uint32_t     responseBufLen   /*0*/)
{
	AtResponse response = AT_RESPONSE_NONE;

	timer.reset();
	timer.start();

    if (gpResponse != NULL){
        printf ("ERROR: response from module not cleared from last time.\r\n");
        gpResponse = NULL;
    }

    do {
        rxTick();
        if (gpResponse != NULL){
            // Got a line, process it
            if ((strncmp(gpResponse, AT_OK, gLenResponse) == 0) && (gLenResponse == (sizeof (AT_OK) - 1))) // -1 to omit 0 of string
            {
                response = AT_RESPONSE_OK;
            }
            else if ((strncmp(gpResponse, AT_ERROR, gLenResponse) == 0) && (gLenResponse == (sizeof (AT_ERROR) - 1))) // -1 to omit 0 of string
            {
                response = AT_RESPONSE_ERROR;
            }
            else if ((pExpected != NULL) && (gLenResponse >= strlen (pExpected)) && (strcmp(gpResponse, pExpected) >= 0))
            {
                response = AT_RESPONSE_STARTS_AS_EXPECTED;
                if (pResponseBuf != NULL)
                {
                    // Copy the response string into pResponseBuf, with a terminator
                    if (gLenResponse > responseBufLen - 1)
                    {
                        gLenResponse = responseBufLen - 1;
                    }
                    memcpy (pResponseBuf, gpResponse, gLenResponse);
                    pResponseBuf[gLenResponse] = 0;
                }
            }
            else
            {
                if (pExpected != NULL)
                {
                    printf ("[modem->waitResponse]  WARNING: unexpected response from module.\r\n");
                    printf ("[modem->waitResponse]  Expected: %s... Received: %.*s \r\n", pExpected, (int) gLenResponse, gpResponse);
                }
                // Reset response pointer
                gpResponse = NULL;
            }
        }
        if (gpResponse == NULL)
        {
            wait_ms(AT_RX_POLL_TIMER_MS);
        }
    } while ( (response == AT_RESPONSE_NONE) && ((timeoutSeconds == 0) || (timer.read() < timeoutSeconds)) );
    // Reset response pointer for next time
    gpResponse = NULL;
    return response;
}


// (5)
// Constructor.  pPortname is a string that defines the serial port where the
// NB-IoT modem is connected.  On Windows the form of a properly escaped string
// must be as follows:
//
// "\\\\.\\COMx"
//
// ...where x is replaced by the COM port number.  So, for COM17, the string
// would be:
//
// "\\\\.\\COM17"
Nbiot::Nbiot(const char * pPortname)
{
    gpResponse   = NULL;
    gpSerialPort = NULL;
    gLenResponse = 0;
    gMatched = 0;
    gLenRx = 0;
    gInitialised = false;

    gpSerialPort = new SerialPort();
    if (gpSerialPort)
    {
    	gInitialised = true;
    	//waitResponse(NULL, DEFAULT_FLUSH_TIMEOUT_SECONDS);
    }
}

// (6)
Nbiot::~Nbiot (){
	delete (gpSerialPort);
}

// (7)
// Connect to the NB-IoT network with optional timeoutSeconds.  If usingSoftRadio
// is true then the connect behaviour is matched to that of SoftRadio, otherwise
// it is matched to that of a real radio.  If timeoutSeconds is zero this function
// will block indefinitely until a connection has been achieved.

bool Nbiot::connect(bool usingSoftRadio /*true*/, time_t timeoutSeconds /*5sec*/)
{
    bool success = false;
    AtResponse response;

    timer.reset();
    timer.start();
    if (gInitialised){
        do {
            if (usingSoftRadio){
                sendPrintf("AT+RAS%s", AT_TERMINATOR);
                response = waitResponse("+RAS:CONNECTED\r\n");
            }
            else{
                sendPrintf("AT+NAS%s", AT_TERMINATOR);
                response = waitResponse("+NAS: Connected (activated)\r\n");
            }


            if (response == AT_RESPONSE_STARTS_AS_EXPECTED){
                waitResponse();
                printf ("Connected to network!\r\n");
                sendPrintf("AT+SMI=1%s", AT_TERMINATOR);
                response = waitResponse("+SMI:OK\r\n");
                if (response == AT_RESPONSE_STARTS_AS_EXPECTED)
                {
                    waitResponse();
                    success = true;
                    printf ("All done!\r\n");
                }
            }
            else
            {
                wait_ms(1000);
            }
        } while ((!success) && ((timeoutSeconds == 0) || (timer.read() < timeoutSeconds)));
    }
    return success;
	//return true;
}



// (8)
// Send the contents of the buffer pMsg, length msgSize, to the NB-IoT network with
// optional timeoutSeconds, waiting for confirmation that the message has been sent.
// If timeoutSeconds is zero this function will block indefinitely until the message
// has been sent.
bool Nbiot::send (char * pMsg, uint32_t msgSize, time_t timeoutSeconds)
{
    bool success = false;
    AtResponse response;
    uint32_t   charCount = 0;

    // Check that the incoming message, when hex coded (so * 2) is not too big
    if ((msgSize * 2) <= sizeof(gHexBuf))
    {
    	charCount = bytesToHexString (pMsg, msgSize, gHexBuf, sizeof(gHexBuf));
    	printf("Sending datagram to network, %d characters: %.*s\r\n", msgSize, (int) msgSize, pMsg);
    	sendPrintf("AT+MGS=%d, %.*s%s", msgSize, charCount, gHexBuf, AT_TERMINATOR);


        // Wait for confirmation
        response = waitResponse("+MGS:OK\r\n");
        if (response == AT_RESPONSE_STARTS_AS_EXPECTED)
        {
            // It worked, wait for the "OK"
            waitResponse();
            // Now wait for the SENT indication
            response = waitResponse("+SMI:SENT\r\n", timeoutSeconds);
            if (response == AT_RESPONSE_STARTS_AS_EXPECTED)
            {
                // All done
                success = true;
                printf ("HOOOOOOOOOORRRRRRRRRRRRRAAAAAAAAAAAAAA.\r\n");
            }
        }
    }
    else
    {
        printf ("[modem->send]  datagram is too long (%d characters when only %d bytes can be sent).\r\n", msgSize, (int) (sizeof (gHexBuf) / 2));
    }
    return success;
}



// (9)
// Poll the NB-IoT modem for received data with optional timeoutSeconds.  If data
// has been received the return value will be non-zero, representing the number of
// bytes received.  Up to msgSize bytes of returned data will be stored at pMsg; any
// data beyond that will be lost.  If timeoutSeconds is zero this function will block
// indefinitely until a message has been received.
// Receive a message from the network
uint32_t Nbiot::receive (char * pMsg, uint32_t msgSize, time_t timeoutSeconds)
{
    int bytesReceived = 0;
    AtResponse response;
    char * pHexStart = NULL;
    char * pHexEnd = NULL;

    printf("[modem->receive] receiving a datagram of up to %d byte(s) from the network...\r\n", msgSize);
    sendPrintf("AT+MGR%s", AT_TERMINATOR);

    response = waitResponse("+MGR:", timeoutSeconds, gHexBuf, sizeof (gHexBuf));

    if (response == AT_RESPONSE_STARTS_AS_EXPECTED)
    {
        if (sscanf(gHexBuf, " +MGR:%d,", &bytesReceived) == 1) // The space in the string is significant,
                                                               // it allows any whitespace characters in the input string
        {
            pHexStart = strchr (gHexBuf, ',');
            if (pHexStart != NULL)
            {
                pHexStart++;
                pHexEnd = strstr (pHexStart, AT_TERMINATOR);
                if ((pHexEnd != NULL) && (pMsg != NULL))
                {
                    hexStringToBytes (pHexStart, pHexEnd - pHexStart, pMsg, msgSize);
                }
            }

            // Wait for the OK at the end
            response = waitResponse("+MGR:OK\r\n");
        }
    }
    return (uint32_t) bytesReceived;
}

// HamedSiasi

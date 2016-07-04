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

bool Nbiot::flag = false;
Nbiot      *Nbiot::obj = NULL;
SerialPort *Nbiot::gpSerialPort = NULL;


Nbiot* Nbiot::getInstance(){
	if(!flag){
		obj = new Nbiot();
	}
	return obj;
}

Nbiot::Nbiot(const char * pPortname)
{
    gpResponse   = NULL;
    gpSerialPort = NULL;
    gLenResponse = 0;
    gMatched = 0;
    gLenRx = 0;
    gInitialised = false;
    flag =true;

    gpSerialPort = SerialPort::getInstance();
    if (gpSerialPort)
    {
    	gInitialised = true;
    	waitResponse(NULL, DEFAULT_FLUSH_TIMEOUT_SECONDS);
    }
}

Nbiot::~Nbiot (){
	flag = false;
	delete (gpSerialPort);
}

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
        //printf("--> %s \r\n", gTxBuf);
        success = gpSerialPort->transmitBuffer((const char *) gTxBuf);
    }

    return success;
}


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


void Nbiot::rxTick()
{
    uint32_t len = getLine (gRxBuf, sizeof (gRxBuf));

    if (len > sizeof(AT_TERMINATOR) - 1) // -1 to omit NULL terminator
    {
        if (gpResponse == NULL)
        {
            gLenResponse = len;
            gpResponse = gRxBuf;
        }
    }
}


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
                sendPrintf("AT+SMI=1%s", AT_TERMINATOR);
                response = waitResponse("+SMI:OK\r\n");
                if (response == AT_RESPONSE_STARTS_AS_EXPECTED)
                {
                    waitResponse();
                    success = true;
                }
            }
            else
            {
                wait_ms(1000);
            }
        } while ((!success) && ((timeoutSeconds == 0) || (timer.read() < timeoutSeconds)));
    }
    return success;
}




bool Nbiot::send (char *pMsg, uint32_t msgSize, time_t timeoutSeconds)
{
    bool        success = false;
    AtResponse  response;
    uint32_t    charCount = 0;
    // Check that the incoming message, when hex coded (so * 2) is not too big
    if ((msgSize * 2) <= sizeof(gHexBuf))
    {
    	charCount = bytesToHexString (pMsg, msgSize, gHexBuf, sizeof(gHexBuf));
    	//printf("AT+MGS=%d, %.*s%s", msgSize, charCount, gHexBuf, AT_TERMINATOR);
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
                success = true;
            }
        }
    }
    else
    {
        printf ("[modem->send]  datagram is too long (%d characters when only %d bytes can be sent).\r\n", msgSize, (int) (sizeof (gHexBuf) / 2));
    }
    return success;
}




uint32_t Nbiot::receive (char * pMsg, uint32_t msgSize, time_t timeoutSeconds)
{
    int bytesReceived = 0;
    AtResponse response;
    char * pHexStart = NULL;
    char * pHexEnd = NULL;

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

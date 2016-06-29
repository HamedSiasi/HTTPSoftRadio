// NB-IoT modem driver for NB-IoT example application

#ifndef _MODEM_DRIVER_H_
#define _MODEM_DRIVER_H_

#include "example-mbedos-blinky/serial_driver.h"

#define MAX_LEN_SEND_STRING              128 // 256
#define DEFAULT_RX_INT_STORAGE           8 // 256
//timeout
#define DEFAULT_CONNECT_TIMEOUT_SECONDS   5
#define DEFAULT_SEND_TIMEOUT_SECONDS      5
#define DEFAULT_RECEIVE_TIMEOUT_SECONDS   5
#define DEFAULT_RESPONSE_TIMEOUT_SECONDS  5
#define DEFAULT_FLUSH_TIMEOUT_SECONDS     1


class Nbiot{
public:
    Nbiot (const char * pPortname = "\\\\.\\COM10");
    ~Nbiot ();

    bool connect (
    		bool    usingSoftRadio = true,
			time_t  timeoutSeconds = DEFAULT_CONNECT_TIMEOUT_SECONDS);

    bool send (
    		char     *pMsg,
			uint32_t  msgSize,
			time_t    timeoutSeconds = DEFAULT_SEND_TIMEOUT_SECONDS);

    uint32_t receive (
    		char      *pMsg,
			uint32_t   msgSize,
			time_t     timeoutSeconds = DEFAULT_RECEIVE_TIMEOUT_SECONDS);

protected:

    typedef enum{
        AT_RESPONSE_NONE,
        AT_RESPONSE_STARTS_AS_EXPECTED,
        AT_RESPONSE_OK,
        AT_RESPONSE_ERROR,
        AT_RESPONSE_OTHER
    } AtResponse;

    char gHexBuf [MAX_LEN_SEND_STRING];
    char gTxBuf  [MAX_LEN_SEND_STRING];
    char gRxBuf  [DEFAULT_RX_INT_STORAGE];
    
    uint32_t gLenRx;
    uint32_t gLenResponse;
    uint8_t  gMatched;
    const char *gpResponse;
    
    SerialPort *gpSerialPort = NULL;
    bool gInitialised;

    bool sendPrintf (const char * pFormat, ...);
    uint32_t getLine (char * pBuf, uint32_t lenBuf);
    void rxTick();
    
    AtResponse waitResponse (
    		const char  *pExpected       = NULL,
    		time_t       timeoutSeconds  = DEFAULT_RESPONSE_TIMEOUT_SECONDS,
            char        *pResponseBuf    = NULL,
			uint32_t     responseBufLen  = 0);
};
#endif

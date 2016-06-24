/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed-drivers/mbed.h"
#include "example-mbedos-blinky/modem_driver.h"


//static uint8_t*  gMsgPacket;
//static uint32_t  gMsgPacketSize;


static bool modem(char *datagram, uint32_t datagramLen){
	bool status = false;
	bool usingSoftRadio = true;

	//printf("%s %d", datagram, (int)datagramLen);
	Nbiot *pModem = NULL;
	if( !(pModem = new Nbiot()) ){
		printf ("Out of Memory !!!\r\n");
	}
	else{
		//printf ("SENT....\r\n");
		status = pModem->connect(usingSoftRadio);
		if (status){
			status = pModem->send ( datagram, datagramLen);
			if(status){
				//printf ("SENT.\r\n");
				// Receive a message from the network
				datagramLen = pModem->receive (datagram, datagramLen);
				if (datagramLen > 0){
					printf ("RX: \"%.*s\".\r\n", datagramLen, datagram);
				}
			}
			else{
				printf ("Failed to send datagram !!!\r\n");
			}
		}
		else{
			printf ("Failed to connect to the network !!! \r\n");
		}
	}
	delete (pModem);
	return status;
}



static void blinky(void) {
    static DigitalOut led(LED1);
    led = !led;
    printf("LED = %d \n\r",led.read());
}

static void msgTest(void) {
	char *str = "HELLO";
	size_t strLen = strlen(str);
	modem( str, (uint32_t)strLen);
}


void app_start(int, char**){
    minar::Scheduler::postCallback(msgTest).period(minar::milliseconds(5000));
}


























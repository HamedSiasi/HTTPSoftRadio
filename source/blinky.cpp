
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
#include "example-mbedos-blinky/HTTPClient.h"

//#define DEBUG

static HTTPClient *http = NULL;

#ifdef DEBUG
static bool checkHeap(int bytes){
	bool retval = false;
	char *p = NULL;
	p = (char*)malloc(bytes);
	if(p){
		retval = true;
	}
	free(p);
	return retval;
}
static void checkStack(void){
	unsigned int counter = 1;
	while(true)
	{
		char array[counter];
		array[counter-1] = 7;
		printf("%d KB\r\n",counter);
		counter++;
		wait(0.1);
	}
	//never arrive here
}
#endif

/*
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
}*/


static void get(void) {
	char str[64]; //512

    // --- GET data ---
	http = new HTTPClient();

    printf("Trying to fetch page... \r\n");
    int ret = http->get("http://mbed.org/media/uploads/donatien/hello.txt", str, 128);
    if (!ret)
    {
      printf("Page fetched successfully - read %d characters \r\n", strlen(str));
      printf("Result: %s \r\n", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d \r\n", ret, http->getHTTPResponseCode());
    }
}

/*
static void post(void) {
	printf("Connecting...\r\n");
	HTTPClient http;
	char str[32]; // 512

	//--- POST data ---
	HTTPMap map;
	HTTPText text(str, 512);
    map.put("Hello", "World");
    map.put("test", "1234");
    printf("Trying to post data... \r\n");
    int ret = http.post("http://httpbin.org/post", map, &text);
    if (!ret)
    {
      printf("Executed POST successfully - read %d characters \r\n", strlen(str));
      printf("Result: %s\n", str);
	}
    else
    {
      printf("Error - ret = %d - HTTP return code = %d \r\n", ret, http.getHTTPResponseCode());
    }
}*/

static void msgTest(void) {
	char *str = "HELLO";
	size_t strLen = strlen(str);
	//modem( str, (uint32_t)strLen);
}

static void blinky(void) {
    static DigitalOut led(LED1);
    led = !led;
    printf("LED = %d \n\r",led.read());
}




void app_start(int, char**)
{
    //minar::Scheduler::postCallback(blinky).period(minar::milliseconds(2000));
    minar::Scheduler::postCallback(get).period(minar::milliseconds(3000));
}




























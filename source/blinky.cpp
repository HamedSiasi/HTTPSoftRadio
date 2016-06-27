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
	printf("Connecting...\n");
	HTTPClient http;
	char str[512];
    // --- GET data ---
    printf("Trying to fetch page...\n");
    int ret = http.get("http://mbed.org/media/uploads/donatien/hello.txt", str, 128);
    if (!ret)
    {
      printf("Page fetched successfully - read %d characters\n", strlen(str));
      printf("Result: %s\n", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n", ret, http.getHTTPResponseCode());
    }
}


/*
static void post(void) {
	printf("Connecting...\n");
	HTTPClient http;
	char str[512];
	//--- POST data ---
	HTTPMap map;
	HTTPText text(str, 512);
    map.put("Hello", "World");
    map.put("test", "1234");
    printf("Trying to post data...\n");
    int ret = http.post("http://httpbin.org/post", map, &text);
    if (!ret)
    {
      printf("Executed POST successfully - read %d characters\n", strlen(str));
      printf("Result: %s\n", str);
	}
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n", ret, http.getHTTPResponseCode());
    }
}*/


static void msgTest(void) {
	char *str = "HELLO";
	size_t strLen = strlen(str);
	//modem( str, (uint32_t)strLen);
}


void app_start(int, char**){
    //minar::Scheduler::postCallback(msgTest).period(minar::milliseconds(5000));
	minar::Scheduler::postCallback(get).period(minar::milliseconds(10000));
}


























/* HTTPClient.h */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** \file
HTTP Client header file
*/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#define HTTP_CLIENT_DEFAULT_TIMEOUT 15000

class HTTPData;

#include "IHTTPData.h"
#include "mbed.h"
#include "example-mbedos-blinky/modem_driver.h"


enum HTTPResult {
  HTTP_PROCESSING, ///<Processing
  HTTP_PARSE,      ///<url Parse error
  HTTP_DNS,        ///<Could not resolve name
  HTTP_PRTCL,      ///<Protocol error
  HTTP_NOTFOUND,   ///<HTTP 404 Error
  HTTP_REFUSED,    ///<HTTP 403 Error
  HTTP_ERROR,      ///<HTTP xxx error
  HTTP_TIMEOUT,    ///<Connection timeout
  HTTP_CONN,       ///<Connection error
  HTTP_CLOSED,     ///<Connection was closed by remote host
  HTTP_OK = 0,     ///<Success
};




class HTTPClient {

public:
  HTTPClient();
  ~HTTPClient();

#if 0 //TODO add header handlers
  /**
  Provides a basic authentification feature (Base64 encoded username and password)
  Pass two NULL pointers to switch back to no authentication
  @param user username to use for authentication, must remain valid durlng the whole HTTP session
  @param user password to use for authentication, must remain valid durlng the whole HTTP session
  */
  void basicAuth(const char* user, const char* password); //Basic Authentification
#endif


  HTTPResult get(
		  const char   *url,
		  IHTTPDataIn  *pDataIn,
		  int           timeout = HTTP_CLIENT_DEFAULT_TIMEOUT); //Blocking

  HTTPResult get(
		  const char *url,
		  char       *result,
		  size_t      maxResultLen,
		  int         timeout = HTTP_CLIENT_DEFAULT_TIMEOUT); //Blocking

  HTTPResult post(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT); //Blocking
  HTTPResult put(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT); //Blocking
  HTTPResult del(const char* url, IHTTPDataIn* pDataIn, int timeout = HTTP_CLIENT_DEFAULT_TIMEOUT); //Blocking

  int getHTTPResponseCode();
  static HTTPClient* getInstance();


private:
  static Nbiot *pModem;

  enum HTTP_METH
  {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
  };

  HTTPResult connect(const char* url, HTTP_METH method, IHTTPDataOut* pDataOut, IHTTPDataIn* pDataIn, int timeout); //Execute request
  HTTPResult recv(char* buf, size_t minLen, size_t maxLen, size_t* pReadLen); //0 on success, err code on failure
  HTTPResult send(char* buf, size_t len = 0); //0 on success, err code on failure
  HTTPResult parseURL(const char* url, char* scheme, size_t maxSchemeLen, char* host, size_t maxHostLen, uint16_t* port, char* path, size_t maxPathLen); //Parse URL

  int m_timeout;
  const char* m_basicAuthUser;
  const char* m_basicAuthPassword;
  int m_httpResponseCode;

  static bool flag;
  static HTTPClient* obj;
};

#include "HTTPText.h"
#include "HTTPMap.h"
#endif

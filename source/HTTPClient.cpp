/* HTTPClient.cpp */
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

#include <cstdio>

#define OK 0
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#define CHUNK_SIZE 128

#include <cstring>
#include "example-mbedos-blinky/HTTPClient.h"

bool HTTPClient::flag = false;
HTTPClient *HTTPClient::obj = NULL;
Nbiot      *HTTPClient::pModem = NULL;

HTTPClient* HTTPClient::getInstance(){
	if(!obj){
		obj = new HTTPClient();
	}
	return obj;
}

HTTPClient::HTTPClient() :m_basicAuthUser(NULL),m_basicAuthPassword(NULL),m_httpResponseCode(0)
{
	flag = true;
	pModem = Nbiot::getInstance();
}

HTTPClient::~HTTPClient()
{
	flag = false;
	delete(pModem);
}

#if 0
void HTTPClient::basicAuth(const char* user, const char* password) //Basic Authentification
{
  m_basicAuthUser = user;
  m_basicAuthPassword = password;
}
#endif

HTTPResult HTTPClient::get(const char* url, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
  return connect(url, HTTP_GET, NULL, pDataIn, timeout);
}

HTTPResult HTTPClient::get(const char* url, char* result, size_t maxResultLen, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
  HTTPText str(result, maxResultLen);
  return get(url, &str, timeout);
}

HTTPResult HTTPClient::post(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
  return connect(url, HTTP_POST, (IHTTPDataOut*)&dataOut, pDataIn, timeout);
}

HTTPResult HTTPClient::put(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
  return connect(url, HTTP_PUT, (IHTTPDataOut*)&dataOut, pDataIn, timeout);
}

HTTPResult HTTPClient::del(const char* url, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
  return connect(url, HTTP_DELETE, NULL, pDataIn, timeout);
}

int HTTPClient::getHTTPResponseCode()
{
  return m_httpResponseCode;
}

#define CHECK_CONN_printf(ret) \
  do{ \
    if(ret) { \
      printf("Connection printfor (%d) \r\n", ret); \
      return HTTP_CONN; \
    } \
  } while(0)

#define PRTCL_printf() \
  do{ \
    printf("Protocol printfor \r\n"); \
    return HTTP_PRTCL; \
  } while(0)




HTTPResult HTTPClient::connect(const char* url, HTTP_METH method, IHTTPDataOut* pDataOut, IHTTPDataIn* pDataIn, int timeout) //Execute request
{
  m_httpResponseCode = 0;
  m_timeout = timeout;

  pDataIn->writeReset();
  if( pDataOut )
  {
    pDataOut->readReset();
  }

  char scheme[8];
  uint16_t port;
  char host[32];
  char path[64];
  //First we need to parse the url (http[s]://host[:port][/[path]]) -- HTTPS not supported (yet?)
  HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
  if(res != HTTP_OK)
  {
    printf("parseURL returned %d \r\n", res);
    return res;
  }

  if(port == 0) //TODO do handle HTTPS->443
  {
    port = 80;
  }

  //printf("Scheme: %s \r\n", scheme);
  //printf("Host:   %s \r\n", host);
  //printf("Port:   %d \r\n", port);
  //printf("Path:   %s \r\n", path);

  //Send request
  printf("Sending request \r\n");
  char buf[CHUNK_SIZE];
  const char* meth = (method==HTTP_GET)?"GET":(method==HTTP_POST)?"POST":(method==HTTP_PUT)?"PUT":(method==HTTP_DELETE)?"DELETE":"";
  snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); //Write request
  printf("Size of buffer %d \r\n", sizeof(buf));
  int ret = send(buf);

  if(ret)
  {
    printf("Could not write request \r\n");
    return HTTP_CONN;
  }

  //Send all headers

  //Send default headers
  printf("Sending headers \r\n");
  if( pDataOut != NULL )
  {
    if( pDataOut->getIsChunked() )
    {
      ret = send("Transfer-Encoding: chunked\r\n");
      CHECK_CONN_printf(ret);
    }
    else
    {
      snprintf(buf, sizeof(buf), "Content-Length: %d\r\n", pDataOut->getDataLen());
      ret = send(buf);
      CHECK_CONN_printf(ret);
    }
    char type[48];
    if( pDataOut->getDataType(type, 48) == HTTP_OK )
    {
      snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", type);
      ret = send(buf);
      CHECK_CONN_printf(ret);
    }
  }

  //Close headers
  printf("Headers sent \r\n");
  ret = send("\r\n");
  CHECK_CONN_printf(ret);

  size_t trfLen;

  //Send data (if available)
  if( pDataOut != NULL )
  {
    printf("Sending data \r\n");
    while(true)
    {
      size_t writtenLen = 0;
      pDataOut->read(buf, CHUNK_SIZE, &trfLen);
      if( pDataOut->getIsChunked() )
      {
        //Write chunk header
        char chunkHeader[16];
        snprintf(chunkHeader, sizeof(chunkHeader), "%X\r\n", trfLen); //In hex encoding
        ret = send(chunkHeader);
        CHECK_CONN_printf(ret);
      }
      else if( trfLen == 0 )
      {
        break;
      }
      if( trfLen != 0 )
      {
        ret = send(buf, trfLen);
        CHECK_CONN_printf(ret);
      }

      if( pDataOut->getIsChunked()  )
      {
        ret = send("\r\n"); //Chunk-terminating CRLF
        CHECK_CONN_printf(ret);
      }
      else
      {
        writtenLen += trfLen;
        if( writtenLen >= pDataOut->getDataLen() )
        {
          break;
        }
      }

      if( trfLen == 0 )
      {
        break;
      }
    }

  }

  //Receive response
  printf("Receiving response \r\n");
  ret = recv(buf, CHUNK_SIZE - 1, CHUNK_SIZE - 1, &trfLen); //Read n bytes
  CHECK_CONN_printf(ret);

  buf[trfLen] = '\0';

  char* crlfPtr = strstr(buf, "\r\n");
  if(crlfPtr == NULL)
  {
    PRTCL_printf();
  }

  int crlfPos = crlfPtr - buf;
  buf[crlfPos] = '\0';

  //Parse HTTP response
  if( sscanf(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &m_httpResponseCode) != 1 )
  {
    //Cannot match string, printfor
    printf("Not a correct HTTP answer : %s \r\n", buf);
    PRTCL_printf();
  }

  if( (m_httpResponseCode < 200) || (m_httpResponseCode >= 300) )
  {
    //Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers
    printf("Response code %d  \r\n", m_httpResponseCode);
    PRTCL_printf();
  }

  printf("Reading headers \r\n");

  memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
  trfLen -= (crlfPos + 2);

  size_t recvContentLength = 0;
  bool recvChunked = false;
  //Now get headers
  while( true )
  {
    crlfPtr = strstr(buf, "\r\n");
    if(crlfPtr == NULL)
    {
      if( trfLen < CHUNK_SIZE - 1 )
      {
        size_t newTrfLen;
        ret = recv(buf + trfLen, 1, CHUNK_SIZE - trfLen - 1, &newTrfLen);
        trfLen += newTrfLen;
        buf[trfLen] = '\0';
        printf("Read %d chars; In buf: [%s]  \r\n", newTrfLen, buf);
        CHECK_CONN_printf(ret);
        continue;
      }
      else
      {
        PRTCL_printf();
      }
    }

    crlfPos = crlfPtr - buf;

    if(crlfPos == 0) //End of headers
    {
      printf("Headers read \r\n");
      memmove(buf, &buf[2], trfLen - 2 + 1); //Be sure to move NULL-terminating char as well
      trfLen -= 2;
      break;
    }

    buf[crlfPos] = '\0';

    char key[32];
    char value[32];

    key[31] = '\0';
    value[31] = '\0';

    int n = sscanf(buf, "%31[^:]: %31[^\r\n]", key, value);
    if ( n == 2 )
    {
      printf("Read header : %s: %s  \r\n", key, value);
      if( !strcmp(key, "Content-Length") )
      {
        sscanf(value, "%d", &recvContentLength);
        pDataIn->setDataLen(recvContentLength);
      }
      else if( !strcmp(key, "Transfer-Encoding") )
      {
        if( !strcmp(value, "Chunked") || !strcmp(value, "chunked") )
        {
          recvChunked = true;
          pDataIn->setIsChunked(true);
        }
      }
      else if( !strcmp(key, "Content-Type") )
      {
        pDataIn->setDataType(value);
      }

      memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
      trfLen -= (crlfPos + 2);

    }
    else
    {
      printf("Could not parse header  \r\n");
      PRTCL_printf();
    }

  }

  //Receive data
  printf("Receiving data  \r\n");
  while(true)
  {
    size_t readLen = 0;

    if( recvChunked )
    {
      //Read chunk header
      bool foundCrlf;
      do
      {
        foundCrlf = false;
        crlfPos=0;
        buf[trfLen]=0;
        if(trfLen >= 2)
        {
          for(; crlfPos < trfLen - 2; crlfPos++)
          {
            if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' )
            {
              foundCrlf = true;
              break;
            }
          }
        }
        if(!foundCrlf) //Try to read more
        {
          if( trfLen < CHUNK_SIZE )
          {
            size_t newTrfLen;
            ret = recv(buf + trfLen, 0, CHUNK_SIZE - trfLen - 1, &newTrfLen);
            trfLen += newTrfLen;
            CHECK_CONN_printf(ret);
            continue;
          }
          else
          {
            PRTCL_printf();
          }
        }
      } while(!foundCrlf);
      buf[crlfPos] = '\0';
      int n = sscanf(buf, "%x", &readLen);
      if(n!=1)
      {
        printf("Could not read chunk length  \r\n");
        PRTCL_printf();
      }

      memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2)); //Not need to move NULL-terminating char any more
      trfLen -= (crlfPos + 2);

      if( readLen == 0 )
      {
        //Last chunk
        break;
      }
    }
    else
    {
      readLen = recvContentLength;
    }

    printf("Retrieving %d bytes  \r\n", readLen);

    do
    {
      pDataIn->write(buf, MIN(trfLen, readLen));
      if( trfLen > readLen )
      {
        memmove(buf, &buf[readLen], trfLen - readLen);
        trfLen -= readLen;
        readLen = 0;
      }
      else
      {
        readLen -= trfLen;
      }

      if(readLen)
      {
        ret = recv(buf, 1, CHUNK_SIZE - trfLen - 1, &trfLen);
        CHECK_CONN_printf(ret);
      }
    } while(readLen);

    if( recvChunked )
    {
      if(trfLen < 2)
      {
        size_t newTrfLen;
        //Read missing chars to find end of chunk
        ret = recv(buf + trfLen, 2 - trfLen, CHUNK_SIZE - trfLen - 1, &newTrfLen);
        CHECK_CONN_printf(ret);
        trfLen += newTrfLen;
      }
      if( (buf[0] != '\r') || (buf[1] != '\n') )
      {
        printf("Format printfor  \r\n");
        PRTCL_printf();
      }
      memmove(buf, &buf[2], trfLen - 2);
      trfLen -= 2;
    }
    else
    {
      break;
    }

  }

  printf("Completed HTTP transaction  \r\n");
  return HTTP_OK;
}


HTTPResult HTTPClient::recv(char* buf, size_t minLen, size_t maxLen, size_t* pReadLen) //0 on success, printf code on failure
{
  printf("Trying to read between %d and %d bytes\r\n", minLen, maxLen);
  size_t readLen = 0;

  bool status = false;
  bool usingSoftRadio = true;

  int ret;
  while(readLen < maxLen)
  {
    if(readLen < minLen)
    {
      printf("Trying to read at most %d bytes \r\n", minLen - readLen);
      ret = pModem->receive (buf + readLen, minLen - readLen);
    }
    else
    {
      printf("Trying to read at most %d bytes \r\n", maxLen - readLen);
      ret = pModem->receive (buf + readLen, maxLen - readLen);
    }

    if( ret > 0)
    {
      readLen += ret;
    }
    else if( ret == 0 )
    {
      break;
    }
    else
    {
      if(!pModem)
      {
        printf("Connection printfor (recv returned %d) \r\n", ret);
        *pReadLen = readLen;
        return HTTP_CONN;
      }
      else
      {
        break;
      }
    }

    if(!pModem)
    {
      break;
    }
  }
  printf("<-- %s\r\n", buf);
  *pReadLen = readLen;
  return HTTP_OK;
}



HTTPResult HTTPClient::send(char* buf, size_t len /*0*/) //0 on success, printf code on failure
{
  if(len == 0)
  {
    len = strlen(buf);
  }
  printf("Trying to write %d bytes \r\n", len);
  size_t writtenLen = 0;

  bool status = false;
  bool usingSoftRadio = true;

  status = pModem->connect(usingSoftRadio);
  if (status)
  {
	  int ret = pModem->send (buf,(uint32_t)len);
	  if(ret > 0)
	  {
		  	  writtenLen += ret;
	  }
	  else if( ret == 0 )
	  {
		  	  printf("Connection was closed by server  \r\n");
		      return HTTP_CLOSED; //Connection was closed by server
	  }
	  else
	  {
		      printf("Connection printfor (send returned %d) \r\n", ret);
		      return HTTP_CONN;
	  }
	  printf("Written %d bytes \r\n", writtenLen);
	  return HTTP_OK;
  }
}

HTTPResult HTTPClient::parseURL(const char* url, char* scheme, size_t maxSchemeLen, char* host, size_t maxHostLen, uint16_t* port, char* path, size_t maxPathLen) //Parse URL
{
  char* schemePtr = (char*) url;
  char* hostPtr = (char*) strstr(url, "://");
  if(hostPtr == NULL)
  {
    printf("Could not find host \r\n");
    return HTTP_PARSE; //URL is invalid
  }

  if( maxSchemeLen < hostPtr - schemePtr + 1 ) //including NULL-terminating char
  {
    printf("Scheme str is too small (%d >= %d) \r\n", maxSchemeLen, hostPtr - schemePtr + 1);
    return HTTP_PARSE;
  }
  memcpy(scheme, schemePtr, hostPtr - schemePtr);
  scheme[hostPtr - schemePtr] = '\0';

  hostPtr+=3;

  size_t hostLen = 0;

  char* portPtr = strchr(hostPtr, ':');
  if( portPtr != NULL )
  {
    hostLen = portPtr - hostPtr;
    portPtr++;
    if( sscanf(portPtr, "%hu", port) != 1)
    {
      printf("Could not find port \r\n");
      return HTTP_PARSE;
    }
  }
  else
  {
    *port=0;
  }
  char* pathPtr = strchr(hostPtr, '/');
  if( hostLen == 0 )
  {
    hostLen = pathPtr - hostPtr;
  }

  if( maxHostLen < hostLen + 1 ) //including NULL-terminating char
  {
    printf("Host str is too small (%d >= %d) \r\n", maxHostLen, hostLen + 1);
    return HTTP_PARSE;
  }
  memcpy(host, hostPtr, hostLen);
  host[hostLen] = '\0';

  size_t pathLen;
  char* fragmentPtr = strchr(hostPtr, '#');
  if(fragmentPtr != NULL)
  {
    pathLen = fragmentPtr - pathPtr;
  }
  else
  {
    pathLen = strlen(pathPtr);
  }

  if( maxPathLen < pathLen + 1 ) //including NULL-terminating char
  {
    printf("Path str is too small (%d >= %d) \r\n", maxPathLen, pathLen + 1);
    return HTTP_PARSE;
  }
  memcpy(path, pathPtr, pathLen);
  path[pathLen] = '\0';

  return HTTP_OK;
}

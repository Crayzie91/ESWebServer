/*
* @brief	Netconn implementation that gets data from filesystem
*
* @note
* Copyright(C) NXP Semiconductors, 2012
* All rights reserved.
*
* @par
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* LPC products.  This software is supplied "AS IS" without any warranties of
* any kind, and NXP Semiconductors and its licensor disclaim any and
* all warranties, express or implied, including all implied warranties of
* merchantability, fitness for a particular purpose and non-infringement of
* intellectual property rights.  NXP Semiconductors assumes no responsibility
* or liability for the use of the software, conveys no license or rights under any
* patent, copyright, mask work right, or any other intellectual property rights in
* or to any products. NXP Semiconductors reserves the right to make changes
* in the software without notification. NXP Semiconductors also makes no
* representation or warranty that such application will be suitable for the
* specified use without further testing or modification.
*
* @par
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors' and its
* licensor's relevant copyrights in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
*/

#include <stdio.h>
#include <string.h>
#include "lwipopts.h"

#include "lwip_fs.h"

#include "../../lwip/inc/lwip/api.h"
#include "../../lwip/inc/lwip/arch.h"
#include "../../lwip/inc/lwip/opt.h"

#if LWIP_NETCONN

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

/* Default file content incase a valid filesystem is not present */
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>Congrats!</title></head>\r\n"
									  "<body>\r\n"
									  "<h1>Welcome to our lwIP HTTP server!</h1>\r\n"
									  "<p>This is a small test page, served by httpserver-netconn.</p>\r\n"
									  "<form action=\"\" method=\"POST\">\r\n"
									  "<h2>Testeingabe:</h2>\r\n"
									  "<input type=\"text\" name=\"test\">"
									  "<button type=\"submit\">Abschicken</button>"
		        					  "</form>\r\n</body>\r\n</html>\r\n";

/* Dynamic header generation based on Filename */
extern int GetHTTP_Header(const char *fName, char *buff);
void netconn_init(void);

/* Function to check if the requested method is supported */
static int supported_method(const char *method)
{
	if (strncmp(method, "GET", 3) == 0)
		return 1;
	if (strncmp(method, "POST", 4) == 0)
		return 1;
	return 0;
}

/* Function to extract version information from URI */
static uint32_t get_version(const char *vstr)
{
	int major = 0, minor = 0;
	sscanf(vstr, "HTTP/%d.%d", &major, &minor);
	return (major << 16) | minor;
}

/** Serve one HTTP connection accepted in the http thread */
static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf, *tbuf;
  u16_t buflen;
  struct file_ds *fs = NULL;
  err_t err;
  static uint8_t file_buffer[1024];
  int len;
  uint32_t req_ver;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);

  if (err != ERR_OK) return;

	netbuf_data(inbuf, (void**)&buf, &buflen);

#ifdef AssignmentDebugHTTP
	printf("\r\nRequest\r\n%.*s \r\n",buflen,buf);
#endif

	if (buflen < 5 || strstr(buf, CRLF) == NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Invalid Request Line\r\n"));
		goto close_and_exit;
	}

	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("HTTPD: Got URI %s\r\n", buf));

	tbuf = strchr(buf, ' ');
	if (tbuf == NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Parse error in Request Line\r\n"));
		goto close_and_exit;
	}
	
	*tbuf++ = 0;
	if (!supported_method(buf)) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Un-supported method: %s\r\n", buf));
		goto close_and_exit;
	}
	buf = tbuf;
	tbuf = strchr(buf, ' ');
	if (tbuf == NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Version string not found: %s\r\n", buf));
	} else {
		*tbuf++ = 0;
		req_ver = get_version(tbuf);
		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("HTTPD: Request version %d.%d\r\n",
			req_ver >> 16, req_ver & 0xFFFF));
	}
	
	tbuf = strchr(buf, 'test');
	if (tbuf != NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Arguements %s in URI ignored\r\n", tbuf));
		*tbuf++ = 0;
	}

	tbuf = strchr(buf, '?');
	if (tbuf != NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Arguements %s in URI ignored\r\n", tbuf));
		*tbuf++ = 0;
	}
	if (strlen(buf) == 1 && *buf == '/') {//XXX:hier wird versucht eine datei zu öffnen, wenn keine exisitiert werden die standard geladen
		fs = fs_open("/web/index.htm");

		if (fs == NULL) {
			/* No home page, send if from buffer */
			netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

			netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
#ifdef AssignmentDebugHTTP
			printf("Response:\r\n %s",http_html_hdr);
			printf("\r\n %s",http_index_html);
#endif
			goto close_and_exit;
		}
	} else {
		fs = fs_open(buf);
	}
	if (fs == NULL) {
		int len;
		LWIP_DEBUGF(HTTPD_DEBUG, ("HTTPD: Unable to open file[%s]\r\n", buf));
		len = GetHTTP_Header(NULL, (char *)file_buffer);
		netconn_write(conn, file_buffer, len, NETCONN_NOCOPY);
		goto close_and_exit;
	}
	
	if (fs->header)
		/* Send the header */
		netconn_write(conn, fs->header, fs->index, NETCONN_NOCOPY);

#ifdef AssignmentDebugHTTP
			printf("\r\nResponse:\r\n%s",fs->header);
#endif

	 /* Read the file now */
	 while ((len = fs_read(fs, (char *)file_buffer, sizeof(file_buffer))) > 0) {
		netconn_write(conn, file_buffer, len, NETCONN_NOCOPY);

#ifdef AssignmentDebugHTTP
			printf("%s",file_buffer);
#endif
	 }
	
  close_and_exit:
  fs_close(fs);
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/** The main function, never returns! */
static void
netconn_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err;
  LWIP_UNUSED_ARG(arg);
  
  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  LWIP_ERROR("http_server: invalid conn", (conn != NULL), return;);
  
  /* Bind to port 80 (HTTP) with default IP address */
  netconn_bind(conn, NULL, 80);
  
  /* Put the connection into LISTEN state */
  netconn_listen(conn);
  
  do {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      http_server_netconn_serve(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);
  LWIP_DEBUGF(HTTPD_DEBUG,
    ("http_server_netconn_thread: netconn_accept received error %d, shutting down",
    err));
  netconn_close(conn);
  netconn_delete(conn);
}

/*********************************************************************//**
 * @brief	Initialization function
 *
 * This function will start the HTTP server and will serve requests received.
 *
 * @return	None
 **********************************************************************/
void
netconn_init(void)
{
  sys_thread_new("http_server_netconn", netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE + 128, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_NETCONN*/

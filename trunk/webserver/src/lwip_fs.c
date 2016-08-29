/*
 * @brief	lwIP Filesystem implementation module
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
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

#include "lwip_fs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "board.h"

#include "httpd_structs.h"

#if defined(LWIP_FATFS_SUPPORT)
#include "../../fatfs/inc/ff.h"
#include "../../fatfs/inc/fs_mem.h"
#include "../../fatfs/inc/ffconf.h"
#include "../../fatfs/inc/diskio.h"
#endif

/**
 * @ingroup EXAMPLES_DUALCORE_LWIP_FS
 * @{
 */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#if defined(LWIP_FATFS_SUPPORT)
static FATFS *Fatfs;	/* File system object */
#endif

/*****************************************************************************
 * Private functions
 ****************************************************************************/
/**
 * Generate the relevant HTTP headers for the given filename and write
 * them into the supplied buffer.
 */
static int
get_http_headers(const char *fName, char *buff)
{
	unsigned int iLoop;
	const char *pszExt = NULL;
	const char *hdrs[4];

	/* Ensure that we initialize the loop counter. */
	iLoop = 0;

	/* In all cases, the second header we send is the server identification
	   so set it here. */
	hdrs[1] = g_psHTTPHeaderStrings[HTTP_HDR_SERVER];

	/* Is this a normal file or the special case we use to send back the
	   default "404: Page not found" response? */
	if (( fName == NULL) || ( *fName == 0) ) {
		hdrs[0] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_FOUND];
		hdrs[2] = g_psHTTPHeaderStrings[DEFAULT_404_HTML];
		goto end_fn;
	}
	/* We are dealing with a particular filename. Look for one other
	    special case.  We assume that any filename with "404" in it must be
	    indicative of a 404 server error whereas all other files require
	    the 200 OK header. */
	if (strstr(fName, "404")) {
		iLoop = HTTP_HDR_NOT_FOUND;
	}
	else if (strstr(fName, "400")) {
		iLoop = HTTP_HDR_BAD_REQUEST;
	}
	else if (strstr(fName, "501")) {
		iLoop = HTTP_HDR_NOT_IMPL;
	}
	else {
		iLoop = HTTP_HDR_OK;
	}
	hdrs[0] = g_psHTTPHeaderStrings[iLoop];

	/* Get a pointer to the file extension.  We find this by looking for the
	     last occurrence of "." in the filename passed. */
	pszExt = strrchr(fName, '.');

	/* Does the FileName passed have any file extension?  If not, we assume it
	     is a special-case URL used for control state notification and we do
	     not send any HTTP headers with the response. */
	if (pszExt == NULL) {
		return 0;
	}

	pszExt++;
	/* Now determine the content type and add the relevant header for that. */
	for (iLoop = 0; (iLoop < NUM_HTTP_HEADERS); iLoop++)
		/* Have we found a matching extension? */
		if (!strcmp(g_psHTTPHeaders[iLoop].extension, pszExt)) {
			hdrs[2] =
				g_psHTTPHeaderStrings[g_psHTTPHeaders[iLoop].headerIndex];
			break;
		}

	/* Did we find a matching extension? */
	if (iLoop == NUM_HTTP_HEADERS) {
		/* No - use the default, plain text file type. */
		hdrs[2] = g_psHTTPHeaderStrings[HTTP_HDR_DEFAULT_TYPE];
	}

end_fn:
	iLoop = strlen(hdrs[0]);
	strcpy(buff, hdrs[0]);
	strcat(buff, hdrs[1]);
	strcat(buff, hdrs[2]);
	return strlen(buff);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Read http header information into a string */
int GetHTTP_Header(const char *fName, char *buff)
{
	return get_http_headers(fName, buff);
}

/* File open function */
struct file_ds *fs_open(const char *name) {
#if defined(LWIP_FATFS_SUPPORT)
	FRESULT res;
	struct file_ds *fds;

	if (!Fatfs) {
		/* One time allocation not to be freed! */
		Fatfs = malloc(sizeof(*Fatfs));
		if (Fatfs == NULL) return NULL;
		res = f_mount(Fatfs,"",1); /* Never fails */
	}

	/* Allocate file descriptor */
	fds = malloc(sizeof(*fds));
	if (fds == NULL) {
		DEBUGSTR("Malloc Failure, Out of Memory!\r\n");
		return NULL;
	}

	res = f_open(&fds->file, name, FA_OPEN_ALWAYS | FA_READ);

	if (res) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("DFS: OPEN: File %s does not exist\r\n", name));
		printf("File %s does not exist\r\n", name);//XXX
		free(fds);
		return NULL;
	}

	fds->fi_valid = 1;
	fds->index = get_http_headers(name, (char *) fds->header);
	fds->len = f_size(&fds->file) + fds->index;
	return fds;
#else
	return 0;
#endif
}


/* File close function */
void fs_close(struct file_ds *fds)
{
	if(fds == NULL)
		return;

#if defined(LWIP_FATFS_SUPPORT)
	if (fds->fi_valid)
		f_close(&fds->file);
#endif
 	free(fds);
}

#if defined(LWIP_FATFS_SUPPORT)
/* File read function */
int fs_read(struct file_ds *fds, char *buffer, int count)
{
	uint32_t i = 0;
	if (f_read(&fds->file, (uint8_t *) buffer, count, &i))
		return 0; /* Error in reading file */
	fds->index += i;
	return i;
}

#else

int fs_read(struct file_ds *file, char *buffer, int count)
{
	return 0;
}

#endif /* defined(LWIP_FATFS_SUPPORT) */

#ifdef LWIP_DEBUG
/* Assert print function */
void assert_printf(char *msg, int line, char *file)
{
	DEBUGOUT("ASSERT: %s at %s:%d\r\n", msg, file, line);
}

/* LWIP str err function */
const char *lwip_strerr(err_t eno)
{
	return "";
}

#endif

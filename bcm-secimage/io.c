/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/* 
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: io.c
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "secimage.h"

/*----------------------------------------------------------------------
 * Name    : ReadHexFile
 * Purpose : Read some data from file of ascii hex characters
 * Input   : fname : file to be read
 *           buf : buffer which is the data desitnation
 *           maxlen : maximum length of data to be read
 * Output  : none
 *---------------------------------------------------------------------*/
int ReadHexFile (char *fname, uint8_t *buf, int maxlen)
{
	FILE *fp = NULL;
	int len = 0, c;

	fp = fopen (fname, "rb");

	if (fp == NULL) {
		printf ("\nUnable to open file: %s\n", fname);
		return SBERROR_DATA_FILE;
	}

	while ((fscanf(fp, "%02x", &c) >= 0) && len < maxlen)
		buf[len++] = c;
	fclose(fp);

	return len;
}

/*----------------------------------------------------------------------
 * Name    : ReadBinaryFile
 * Purpose : Read some data from file of raw binary
 * Input   : fname : file to be read
 *           buf : buffer which is the data desitnation
 *           maxlen : maiximum length of data to be read
 * Output  : none
 *---------------------------------------------------------------------*/
int ReadBinaryFile (char *fname, uint8_t *buf, int maxlen)
{
	FILE *fp = NULL;
	int len = 0;

	fp = fopen (fname, "rb");
	if (fp == NULL) {
		printf ("\nUnable to open file: %s\n", fname);
		return SBERROR_DATA_FILE;
	}

	//printf("fname=%s, len=%d\n", fname, maxlen);
	len = fread(buf, 1, maxlen, fp);

	fclose(fp);

	return len;
}

/*----------------------------------------------------------------------
 * Name    : SecImage_GetFileSize
 * Purpose : Return the size of the file
 * Input   : file: FILE * to the file to be processed
 * Output  : none
 *---------------------------------------------------------------------*/
long SecImage_GetFileSize (char *filename)
{
	int fd;
	struct stat stbuf;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return SBERROR_DATA_FILE;
	if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode)))
		return SBERROR_DATA_FILE;

	return (long)(stbuf.st_size);

}

/*----------------------------------------------------------------------
 * Name    : DataRead
 * Purpose : Read all the data from a file
 * Input   : filename : file to be read
 *           buf : buffer which is the data desitnation
 *           length : length of data to be read
 * Output  : none
 *---------------------------------------------------------------------*/
int DataRead(char *filename, uint8_t *buf, int *length)
{
	FILE *file;
	long len = *length;

	file = fopen (filename, "rb");
	if (file == NULL) {
		printf ("\nUnable to open file: %s\n", filename);
		return SBERROR_DATA_FILE;
	}
	len = SecImage_GetFileSize(filename);
	if (len < 0)
		return SBERROR_DATA_FILE;

	if (len < *length)
		*length = len;
	else
		len = *length; /* Do not exceed the maximum length of the buffer */
	if (fread((uint8_t *)buf, 1, len, file) != len) {
		printf ("\nError reading data from file: %s\n", filename);
		return SBERROR_DATA_READ;
	}
	fclose(file);
	return STATUS_OK;
}
			
/*----------------------------------------------------------------------
 * Name    : DataWrite
 * Purpose : Write some bianry data to a file
 * Input   : filename : file to be written
 *           buf : buffer which is the data source
 *           length : length of data to be written
 * Output  : none
 *---------------------------------------------------------------------*/
int DataWrite(char *filename, char *buf, int length) 
{
	FILE *file;
	
	file = fopen (filename, "wb");	
	if (file == NULL) {
		printf ("\nUnable to open output file %s\n", filename);
		return SBERROR_DATA_FILE;
	}
	if (fwrite (buf, 1, length, file) < length) {
		printf ("\nUnable to write to output file %s\n", filename);
		return SBERROR_DATA_READ;
	}
//	else printf ("Writing %d bytes to file %s\n", length, filename);
	fclose(file);
	return STATUS_OK;
}

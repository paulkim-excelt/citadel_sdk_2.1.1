#ifndef _ICONV_H_
#define _ICONV_H_

#ifndef iconv_t
typedef int iconv_t;
#endif
#ifndef size_t
typedef unsigned int size_t;
#endif

size_t iconv(iconv_t cd __attribute__((unused)), char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);

iconv_t iconv_open(const char *tocode __attribute__((unused)), const char *fromcode __attribute__((unused)));

int iconv_close(iconv_t cd __attribute__((unused)));
				   
#endif //_ICONV_H_

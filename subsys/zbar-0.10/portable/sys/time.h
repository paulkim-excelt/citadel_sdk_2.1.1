#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <_ansi.h>
#include <sys/cdefs.h>
#include <sys/_timeval.h>
#include <sys/types.h>
#include <sys/timespec.h>

int _EXFUN(gettimeofday, (struct timeval *__restrict __p, void *__restrict __tz));


#endif /*_SYS_TIME_H_*/

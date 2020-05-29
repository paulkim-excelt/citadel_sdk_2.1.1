/* ANSI C namespace clean utility typedefs */

/* This file defines various typedefs needed by the system calls that support
   the C library.  Basically, they're just the POSIX versions with an '_'
   prepended.  Targets shall use <machine/_types.h> to define their own
   internal types if desired.

   There are three define patterns used for type definitions.  Lets assume
   xyz_t is a user type.

   The internal type definition uses __machine_xyz_t_defined.  It is defined by
   <machine/_types.h> to disable a default definition in <sys/_types.h>. It
   must not be used in other files.

   User type definitions are guarded by __xyz_t_defined in glibc and
   _XYZ_T_DECLARED in BSD compatible systems.
*/

#ifndef	_SYS__TYPES_H
#define _SYS__TYPES_H

typedef long _off_t;
typedef int __pid_t;
typedef short __dev_t;
typedef unsigned short __uid_t;
typedef unsigned short __gid_t;
typedef unsigned long __id_t;
typedef unsigned short __ino_t;
typedef long __key_t;
typedef  unsigned short  __nlink_t;
typedef unsigned long __mode_t;
typedef long long _off64_t;
typedef _off_t __off_t;
typedef _off64_t __loff_t;
typedef  unsigned long   __useconds_t;   /* microseconds (unsigned) */
typedef  long            __suseconds_t;  /* microseconds (signed) */
typedef signed long __int64_t;
typedef long __blksize_t;
typedef long __blkcnt_t;

#define  _CLOCKID_T_     unsigned long
typedef  _CLOCKID_T_     __clockid_t;

#define  _CLOCK_T_       unsigned long   /* clock() */
#define  _TIME_T_        long            /* time() */

#define  _TIMER_T_       unsigned long
typedef  _TIMER_T_       __timer_t;

typedef __dev_t         dev_t;          /* device number or struct cdev */
typedef __uid_t         uid_t;          /* user id */
typedef __gid_t         gid_t;          /* group id */
typedef __pid_t         pid_t;          /* process id */
typedef __key_t         key_t;          /* IPC key */
typedef __nlink_t       nlink_t;        /* link count */
typedef __clockid_t     clockid_t;
typedef __timer_t       timer_t;
typedef __useconds_t    useconds_t;     /* microseconds (unsigned) */
typedef __suseconds_t   suseconds_t;
typedef __int64_t       sbintime_t;
typedef   __ino_t         ino_t;
typedef   __blksize_t     blksize_t;
typedef   __blkcnt_t      blkcnt_t;

#endif	/* _SYS__TYPES_H */

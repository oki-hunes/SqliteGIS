/* Minimal cpl_config.h for Windows MSVC build */
#ifndef CPL_CONFIG_H
#define CPL_CONFIG_H

/* Size definitions for basic types */
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_VOIDP 8
#define SIZEOF_SIZE_T 8

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Windows specific */
#define VSI_STAT64 _stat64
#define VSI_STAT64_T __stat64

/* CPL DLL export */
#ifndef CPL_DLL
#if defined(_MSC_VER)
#define CPL_DLL     __declspec(dllimport)
#else
#define CPL_DLL
#endif
#endif

#ifndef CPL_C_START
#ifdef __cplusplus
#define CPL_C_START extern "C" {
#define CPL_C_END }
#else
#define CPL_C_START
#define CPL_C_END
#endif
#endif

#define CPL_UNSTABLE_API

#endif /* CPL_CONFIG_H */

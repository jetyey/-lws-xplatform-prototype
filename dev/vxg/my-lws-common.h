#ifndef _MY_LWS_COMMON_H
#define _MY_LWS_COMMON_H
/************************************************************************/
/* Global includes                                                      */
/************************************************************************/
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _MACOSX_SOURCE
# include <pthread.h>
#endif  // _MACOSX_SOURCE

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#ifdef _WINDOWS_SOURCE
# define snprintf(a,b,c,...)            _snprintf_s(a, b, b, c, __VA_ARGS__)
# define strcasecmp                     _stricmp
# define strdup                         _strdup
# define open                           _open
# define write                          _write
# define close                          _close
# define slash                          "\\"
# define slash_char                     '\\'
#else   // _WINDOWS_SOURCE
# define slash                          "/"
# define slash_char                     '/'
#endif	// _WINDOWS_SOURCE

/************************************************************************/
/* Classes declaration                                                  */
/************************************************************************/

#ifdef __cpluplus
extern "C" {
#endif  // __cpluplus

/** @enum ProtocolType */
typedef enum ProtocolType_t
{
  PROTOCOL_UNKNOWN  = 0x00
, PROTOCOL_HTTP     = 0x01
, PROTOCOL_CHAT     = 0x02
, PROTOCOL_UPLOAD   = 0x04
}
ProtocolType;
  
#ifdef __cpluplus
};
#endif  // __cpluplus

/************************************************************************/
/* Functions declaration                                                */
/************************************************************************/

//=======================================================================
extern "C" char* UpgradeBuffer(char** ioppBuffer, size_t* iopBufferSize, const size_t iNewSize);
//=======================================================================

#endif  // _MY_LWS_COMMON_H

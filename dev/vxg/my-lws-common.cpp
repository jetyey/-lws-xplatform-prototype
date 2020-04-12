/************************************************************************/
/* Global includes                                                      */
/************************************************************************/

/************************************************************************/
/* Local include                                                        */
/************************************************************************/
#include "my-lws-common.h"

/************************************************************************/
/* Defines                                                              */
/************************************************************************/

/************************************************************************/
/* Functions implementation                                             */
/************************************************************************/

//=======================================================================
char* UpgradeBuffer(char** ioppBuffer, size_t* iopBufferSize, const size_t iNewSize)
//=======================================================================
{
  if( (nullptr == ioppBuffer) || (nullptr == iopBufferSize) ) return nullptr;

  // Upgrade buffer size.
  if( iNewSize < *iopBufferSize ) return *ioppBuffer;

  // Save old buffer address, in case.
  char* l_pBufferOld = *ioppBuffer;
        
  // Null-terminated length.
  size_t l_Len = iNewSize + 1;

  // Realloc string buffer.
  if( nullptr != (*ioppBuffer = (char *)realloc(l_pBufferOld, l_Len)) )
  {
    // Log.
    fprintf(stdout, "Realloc len:%zd <-> new len:%zd (+%zd)\n", *iopBufferSize, l_Len, (l_Len - *iopBufferSize));
    fflush(stdout);
          
    // Update current buffer size.
    *iopBufferSize = l_Len;
  }
  else
  {
    // Reset size.
    *iopBufferSize = 0;
          
    // Delete string buffer.
    free(l_pBufferOld);
    l_pBufferOld = nullptr;
  }

  // Done.
  return *ioppBuffer;
}

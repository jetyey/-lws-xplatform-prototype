/************************************************************************/
/* Global include                                                       */
/************************************************************************/
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
# include <pthread.h>
#endif  // _MACOSX_SOURCE

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#ifdef _WINDOWS_SOURCE
# define snprintf(a,b,c,...)    _snprintf_s(a, b, b, c, __VA_ARGS__)
# define strcasecmp             _stricmp
# define strdup                 _strdup
#endif	// _WINDOWS_SOURCE

/************************************************************************/
/* Classes declaration                                                  */
/************************************************************************/

#ifdef __cpluplus
extern "C" {
#endif  // __cpluplus

/** @struct ServiceThreadStruct */
struct ServiceThreadStruct
{
  /// Context.
  lws_context* _pContext;
  
  /// Broken flag.
  int _Break;
  
  /// Return code.
  int _Ret;
};

/** @struct ServerConfigStruct */
struct ServerConfigStruct
{
  /// Port number.
  int _Port;

  /// Connection count.
  size_t _ClientCounter;
  
  /// Client max.
  int _lClientMax;

  /// Client array.
  lws** _ppClient;
};
  
/** @struct ConnectionDataStruct */
struct ConnectionDataStruct
{
  /// Client identifier.
  int _Id;
  
  /// Buffer.
  char* _pBuffer;
  
  /// Buffer length.
  size_t _lBuffer;
  
  /// Client name.
  char _aClientAddr[BUFSIZ];
};

#ifdef __cpluplus
};
#endif  // __cpluplus

/************************************************************************/
/* Variables declaration                                                */
/************************************************************************/

/************************************************************************/
/* Functions declaration                                                */
/************************************************************************/

//=======================================================================
extern "C" int fProtocolChat  ( struct lws* wsi
                              , enum lws_callback_reasons reason
                              , void* user
                              , void* in
                              , size_t len);
//=======================================================================

//=======================================================================
#ifdef _WINDOWS_SOURCE
extern "C" DWORD WINAPI fThread(LPVOID ipParam);
#else   // _WINDOWS_SOURCE
extern "C" void* fThread(void* ipParam);
#endif  // _WINDOWS_SOURCE
//=======================================================================

/************************************************************************/
/************************************************************************/

//=======================================================================
int ClientConnectAdd(ServerConfigStruct& irConfig, const lws* ipWebSocket);
//=======================================================================

//=======================================================================
int ClientConnectRemove(ServerConfigStruct& irConfig, const lws* ipWebSocket);
//=======================================================================

//=======================================================================
int ClientConnectDispatchMessageToAllButMe(ServerConfigStruct& irConfig, const lws* ipWebSocket, const char* ipMessage, const size_t ilMessage);
//=======================================================================

/************************************************************************/
/* Programm core                                                        */
/************************************************************************/

//=======================================================================
int main( int argc, char *argv[] )
//=======================================================================
{
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("Current working dir: %s\n", cwd);
  } else {
    perror("getcwd() error");
    
    // Oops.
    return EXIT_FAILURE;
  }
#endif  // _MACOSX_SOURCE
  
  // Array of protocols.
  struct lws_protocols l_aProtocols[] =
  {
    { "protocol-chat"     , fProtocolChat   , sizeof(ConnectionDataStruct)  , 0 }
    /**/
  , { nullptr             , nullptr         ,  0                            , 0 }
  };
  
  ServerConfigStruct l_Config;
  
  // Reset.
  memset(&l_Config, 0, sizeof(l_Config));
  
  // Initialize configuration.
  l_Config._Port = 8000;
  l_Config._lClientMax = 10;
  
  // Create array of clients.
  if( nullptr == (l_Config._ppClient = (lws **)malloc((l_Config._lClientMax * sizeof(lws *)))) ) return EXIT_FAILURE;
  
  // Reset.
  for( int c = 0; c < l_Config._lClientMax; c++ ) l_Config._ppClient[c] = nullptr;
  
  // Context information structure.
  struct lws_context_creation_info info;
  
  // Reset.
  memset(&info, 0, sizeof(info));
  
  // Initialize context information.
  info.port = l_Config._Port;
  info.protocols = l_aProtocols;
  info.user = &l_Config;
  info.gid = -1;
  info.uid = -1;
  
  // Create the websocket context.
  struct lws_context* context = lws_create_context(&info);
  if( nullptr == context ) return EXIT_FAILURE;
  
  ServiceThreadStruct l_ServiceThread;
  
  // Reset.
  memset(&l_ServiceThread, 0x00, sizeof(l_ServiceThread));
  
  // Initialize.
  l_ServiceThread._Break = 0;
  l_ServiceThread._pContext = context;
  l_ServiceThread._Ret = 0;

  int l_Ret = 0;
 
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
  pthread_t l_hThread;
#else   // _MACOSX_SOURCE
  DWORD l_ThreadId;
  HANDLE l_hThread = NULL;
#endif  // _MACOSX_SOURCE

  size_t l_Len = 0;
  
  char l_aStdInBuffer[BUFSIZ];

  do
  {
    // Create thread.
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
    if( 0 != (l_Ret = pthread_create  ( &l_hThread
                                      , nullptr
                                      , fThread
                                      , &l_ServiceThread)) )
#else   // _MACOSX_SOURCE
    if( NULL == (l_hThread = CreateThread ( NULL
                                          , 0
                                          , fThread
                                          , &l_ServiceThread
                                          , 0
                                          , &l_ThreadId)) )
#endif  // _MACOSX_SOURCE
    {
      // Get out.
      break;
    }

    fprintf(stdout, "Press 'q' to exit:\n");
    fflush(stdout);
    
    // Wait key to be pressed.
    if( nullptr == fgets(l_aStdInBuffer, BUFSIZ, stdin) )
    {
      // Log.
      fprintf(stdout, "Unexpected break (%s)\n", l_aStdInBuffer);
      fflush(stdout);
      
#ifdef __APPLE_CC__
      // When a breakpoint is set/unset, Xcode closes
      // stdin, so do not break in such a situation.
      continue;
#else   // __APPLE_CC__
      // Get out.
      break;
#endif  // __APPLE_CC__
    }
    
    // Remove the line break.
    if( '\n' == (l_aStdInBuffer[(l_Len = (strlen(l_aStdInBuffer) - 1))]) )
    { l_aStdInBuffer[l_Len] = '\0'; }
    
    // Check exit condition.
    if( 0 == strcasecmp(l_aStdInBuffer, "q") )
    {
      // Log.
      fprintf(stdout, "Broken with '%s'\n", l_aStdInBuffer);
      fflush(stdout);
                       
      // Break;
      l_ServiceThread._Break = -1;
    }
  }
  while( 0 == l_ServiceThread._Break );
 
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
  if( 0 == l_Ret )
  {
    // Join thread.
    pthread_join(l_hThread, nullptr);
  }
#else   // _MACOSX_SOURCE
  if( NULL != l_hThread )
  {
    // Close handle.
    CloseHandle(l_hThread);
    l_hThread = NULL;
  }
#endif  // _MACOSX_SOURCE
  
  // Destroy the websocket context.
  lws_context_destroy(context);
  context = nullptr;

  // Delete client array.
  free(l_Config._ppClient);
  l_Config._ppClient = nullptr;
  
  // Done.
  return EXIT_SUCCESS;
}

/************************************************************************/
/* Functions implementation                                             */
/************************************************************************/

//=======================================================================
int ClientConnectAdd(ServerConfigStruct& irConfig, const lws* ipWebSocket)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) ) return l_Ret;
  
  int c = 0;
  
  // Walk client list.
  for( ; c < irConfig._lClientMax; c++ )
  {
    // Existing slot.
    if( ipWebSocket == irConfig._ppClient[c] )
    {
      // Log.
      fprintf(stdout, "Already connected.\n");
      fflush(stdout);
      
      // Found.
      break;
    }
    
    // New slot.
    if( nullptr == irConfig._ppClient[c] )
    {
      irConfig._ppClient[c] = (lws *)ipWebSocket;
      
      // Found.
      break;
    }
  }
  
  // No more clients.
  if( c == irConfig._lClientMax )
  {
    // Log.
    fprintf(stdout, "Max connection reached.\n");
    fflush(stdout);
    
    // Refuse session.
    l_Ret = -1;
  }
  else
  {
    // Ok.
    l_Ret = 0;
  }
  
  // Done.
  return l_Ret;
}

//=======================================================================
int ClientConnectRemove(ServerConfigStruct& irConfig, const lws* ipWebSocket)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) ) return l_Ret;
  
  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Client found.
    if( ipWebSocket == irConfig._ppClient[c] )
    {
      // Reset.
      irConfig._ppClient[c] = nullptr;
      
      // Ok.
      l_Ret = 0;
      
      // Found.
      break;
    }
  }
  
  // Done.
  return l_Ret;
}

//=======================================================================
int ClientConnectDispatchMessageToAllButMe(ServerConfigStruct& irConfig, const lws* ipWebSocket, const char* ipMessage, const size_t ilMessage)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) || (nullptr == ipMessage) ) return l_Ret;
  
  unsigned char* buf = nullptr;

  // Create message envelope.
  if (nullptr == (buf = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + ilMessage + LWS_SEND_BUFFER_POST_PADDING))) return l_Ret;

  // Copy letter.
  memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], ipMessage, ilMessage);

  // Reset.
  l_Ret = 0;

  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Skip myself.
    if( (nullptr == irConfig._ppClient[c]) || (ipWebSocket == irConfig._ppClient[c]) ) continue;
    
    // Send response.
    size_t l_LenSent = lws_write(irConfig._ppClient[c], &buf[LWS_SEND_BUFFER_PRE_PADDING], ilMessage, LWS_WRITE_TEXT);

    if( l_LenSent != ilMessage )
    {
      // Oops.
      l_Ret = -1;
      
      // Log.
      fprintf(stdout, "Sent size error : '%zd' (sent:%zd)\n", ilMessage, l_LenSent);
      fflush(stdout);
    }
  }
  
  // Delete string buffer.
  free(buf);
  buf = nullptr;

  // Done.
  return l_Ret;
}

/************************************************************************/
/************************************************************************/

//=======================================================================
#ifdef _WINDOWS_SOURCE
DWORD WINAPI fThread(LPVOID ipParam)
#else   // _WINDOWS_SOURCE
void* fThread(void* ipParam)
#endif  // _WINDOWS_SOURCE
//=======================================================================
{
#ifdef _WINDOWS_SOURCE
  DWORD l_Ret = -1;
#endif  // _WINDOWS_SOURCE

  // Thread parameter.
  ServiceThreadStruct* l_pServiceThread = (ServiceThreadStruct *)(ipParam);
#ifdef _WINDOWS_SOURCE
  if( nullptr == l_pServiceThread ) return l_Ret;
#else   // _WINDOWS_SOURCE
  if( nullptr == l_pServiceThread ) return nullptr;
#endif  // _WINDOWS_SOURCE

  // Infinite loop.
  do
  {
    // libwebsockets is single thread, cycle through events here.
    if( 0 != (l_pServiceThread->_Ret = lws_service(l_pServiceThread->_pContext, /* timeout_ms = */ 500)) )
    {
      // Log.
      fprintf(stdout, "Service broken (code:%d).\n", l_pServiceThread->_Ret);
      fflush(stdout);
    }
    
    // Wait for break.
    //fprintf(stdout, "%ld breath (%d)\n", time(nullptr), l_pServiceThread->_Ret);
    //fflush(stdout);
  }
  while( 0 == l_pServiceThread->_Break );
  
  // Get rid of the WebSockets server.
  lws_cancel_service(l_pServiceThread->_pContext);
  
  // Done.
#ifdef _WINDOWS_SOURCE
  return l_Ret;
#else   // _WINDOWS_SOURCE
  return nullptr;
#endif  // _WINDOWS_SOURCE
};

//=======================================================================
extern "C" int fProtocolChat  ( struct lws* wsi
                              , enum lws_callback_reasons reason
                              , void* user
                              , void* in
                              , size_t len)
//=======================================================================
{
  int l_Ret = 0;
  
  if( nullptr == wsi ) return l_Ret;
  
  // Context.
  lws_context* l_pContext = lws_get_context(wsi);
  if( nullptr == l_pContext ) return l_Ret;
  
  // Server configuration structure.
  ServerConfigStruct* l_pConfig = (ServerConfigStruct *)lws_context_user(l_pContext);
  if( nullptr == l_pConfig ) return l_Ret;

  ConnectionDataStruct* l_pData = (ConnectionDataStruct *)(user);
  
  // Do the right thing.
  switch ( reason )
  {
  case LWS_CALLBACK_PROTOCOL_INIT:
    {
      // Log.
      fprintf(stdout, "Protocol initialization (%p).\n", l_pData);
      fflush(stdout);
    }
    break;
  case LWS_CALLBACK_ESTABLISHED:
    {
      if( nullptr == l_pData ) break;
   
      char l_aUri[64];
      
      // Parse request URI.
      lws_hdr_copy(wsi, l_aUri, sizeof(l_aUri), WSI_TOKEN_GET_URI);
      
      // Log.
      fprintf(stdout, "Uri: '%s'.\n", l_aUri);
      fflush(stdout);

      // Add client.
      if( 0 != ClientConnectAdd(*l_pConfig, wsi) )
      {
        // Oops.
        break;
      }
      
      // Log.
      fprintf(stdout, "Connection established (%p).\n", wsi);
      fflush(stdout);
      
      // Get client addresses.
      if( nullptr == lws_get_peer_simple(wsi, l_pData->_aClientAddr, sizeof(l_pData->_aClientAddr)) )
      {
        // Log.
        fprintf(stdout, "Network connection error from '%s'.\n", l_pData->_aClientAddr);
        fflush(stdout);
        
        // Close session.
        l_Ret = -1;
        
        // Get out.
        break;
      }

      // Next client identifier.
      l_pData->_Id = (int)l_pConfig->_ClientCounter++;
    }
    break;
  case LWS_CALLBACK_WSI_DESTROY:
    {
      // Log.
      fprintf(stdout, "LWS_CALLBACK_WSI_DESTROY.\n");
      fflush(stdout);
    }
    break;
  case LWS_CALLBACK_CLOSED:
    {
      if( nullptr == l_pData ) break;
   
      // Remove client.
      ClientConnectRemove(*l_pConfig, wsi);
      
      // Log.
      fprintf(stdout, "Connection closed (%p, %zd).\n", l_pData, l_pData->_lBuffer);
      fflush(stdout);
      
      // Free buffer string.
      free(l_pData->_pBuffer);
      l_pData->_pBuffer = nullptr;
    }
    break;
  case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
    {
      char l_aClientName[BUFSIZ];
      char l_aClientIP[BUFSIZ];
      
      // Get client addresses.
      lws_get_peer_addresses(wsi, (int)(long)in, l_aClientName, sizeof(l_aClientName) ,l_aClientIP, sizeof(l_aClientIP));
 
      // Log.
      fprintf(stdout, "Network connect from %s (%s).\n", l_aClientName, l_aClientIP);
      fflush(stdout);
      
      // if non-zero returned here, connection is killed.
      //return -1;
    }
    break;
  case LWS_CALLBACK_RECEIVE:
    {
      if( nullptr == l_pData ) break;
      
      // Upgrade buffer size.
      if( l_pData->_lBuffer < len )
      {
        // Save old buffer address, in case.
        char* l_pBufferOld = l_pData->_pBuffer;
        
        // Realloc string buffer.
        if( nullptr != (l_pData->_pBuffer = (char *)realloc(l_pData->_pBuffer, len)) )
        {
          // Log.
          fprintf(stdout, "Realloc len:%zd <-> new len:%zd\n", l_pData->_lBuffer, len);
          fflush(stdout);
          
          // Update current buffer size.
          l_pData->_lBuffer = len;
        }
        else
        {
          // Reset size.
          l_pData->_lBuffer = 0;
          
          // Delete string buffer.
          free(l_pBufferOld);
          l_pBufferOld = nullptr;
          
          // Get out.
          break;
        }
      }
      
      // Copy incoming data.
      memcpy(l_pData->_pBuffer, in, len);
      
      // Log.
      fprintf(stdout, "Client %03d/%03d [%s] - Request '%s' (len:%zd)\n", l_pData->_Id, (int)l_pConfig->_ClientCounter, l_pData->_aClientAddr, l_pData->_pBuffer, l_pData->_lBuffer);
      fflush(stdout);
      
      // Get notified as soon as.
      lws_callback_on_writable(wsi);
    }
    break;
  case LWS_CALLBACK_SERVER_WRITEABLE:
    {
      if( (nullptr == l_pData) || (nullptr == l_pData->_pBuffer) || ('\0' == *l_pData->_pBuffer) ) break;

      char l_aResponse[BUFSIZ];

      // Format response.
      size_t l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "%s (I am %d)", l_pData->_pBuffer, l_pData->_Id);

      // Reset message.
      *l_pData->_pBuffer = '\0';
      
      // Dispatch response.
      ClientConnectDispatchMessageToAllButMe(*l_pConfig, wsi, l_aResponse, (l_LenResponse + 1));
    }
    break;
  default:
    {
    }
  }
  
  // Done.
  return l_Ret;
}

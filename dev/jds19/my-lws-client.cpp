/************************************************************************/
/* Global includes                                                      */
/************************************************************************/
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#ifdef _WINDOWS_SOURCE
# define strcasecmp   _stricmp
# define strdup       _strdup
#endif  // _WINDOWS_SOURCE

/************************************************************************/
/* Classes declaration                                                  */
/************************************************************************/

#ifdef __cpluplus
extern "C" {
#endif  // __cpluplus

/** @struct ClientConfigStruct */
struct ClientConfigStruct
{
  /// Port number.
  int _Port;
  
  /// Protocols
  const lws_protocols* _pProtocols;
  
  /// Client object.
  lws* _pClient;
  
  /// Message.
  char* _pMessage;
};

/** @struct ServiceThreadStruct */
struct ServiceThreadStruct
{
  /// Context.
  lws_context* _pContext;
  
  /// Broken flag.
  int _Break;
  
  /// Return code.
  int _Ret;

  /// Client configuration.
  ClientConfigStruct* _pConfig;
};
  
#ifdef __cpluplus
};
#endif  // __cpluplus

/************************************************************************/
/* Functions declaration                                                */
/************************************************************************/

//=======================================================================
extern "C" lws* ConnectClient ( lws_context* ipContext
                              , const int iPort
                              , const char* ipProtocolName);
//=======================================================================

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
    { "protocol-chat"     , fProtocolChat   , 0  , 0 }
    /**/
  , { nullptr             , nullptr         ,  0 , 0 }
  };
  
  ClientConfigStruct l_Config;
  
  // Reset.
  memset(&l_Config, 0, sizeof(l_Config));
  
  // Initialize configuration.
  l_Config._Port = 8000;
  l_Config._pProtocols = l_aProtocols;
  
  struct lws_context_creation_info info;
  
  // Reset context information structure.
  memset(&info, 0, sizeof(info));
  
  // Initialize.
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = l_Config._pProtocols;
  info.user = &l_Config;
  info.gid = -1;
  info.uid = -1;
  
  ServiceThreadStruct l_ServiceThread;
  
  // Reset.
  memset(&l_ServiceThread, 0x00, sizeof(l_ServiceThread));
  
  // Initialize.
  l_ServiceThread._Break = 0;
  l_ServiceThread._Ret = 0;
  l_ServiceThread._pConfig = &l_Config;
  
  // Create the websocket context.
  if( nullptr == (l_ServiceThread._pContext = lws_create_context(&info)) ) return EXIT_FAILURE;
  
  // Connect to the server, if need be.
  if( nullptr == (l_Config._pClient = ConnectClient ( l_ServiceThread._pContext
                                                    , l_Config._Port
                                                    , l_Config._pProtocols[0].name)) ) return EXIT_FAILURE;

  int l_Ret = 0;
  
#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
  pthread_t l_hThread;
#else   // _MACOSX_SOURCE
  DWORD l_ThreadId;
  HANDLE l_hThread = NULL;
#endif  // _MACOSX_SOURCE
  
  size_t l_Len = 0;
  
  char l_aStdInBuffer[BUFSIZ];

#if defined(_MACOSX_SOURCE) || defined(_LINUX_SOURCE)
  // Create thread.
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
#endif  // _MACOSX_SOURC
  {
    // Get out.
    return EXIT_FAILURE;
  }

  do
  {
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
    
    // Free string buffer.
    free(l_Config._pMessage);
    l_Config._pMessage = nullptr;
    
    // Prepare message.
    l_Config._pMessage = strdup(l_aStdInBuffer);
    
    // Write to server.
    lws_callback_on_writable(l_Config._pClient);
    
    // Lock.
    /* TODO */
    
    // libwebsockets is single thread, cycle through events here.
    if( 0 != (l_ServiceThread._Ret = lws_service(l_ServiceThread._pContext, /* timeout_ms = */ 50)) )
    {
      // Log.
      fprintf(stdout, "Service broken (code:%d).\n", l_ServiceThread._Ret);
      fflush(stdout);
    }
    
    // Unlock.
    /* TODO */
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
  
  // Get rid of the WebSockets server.
  lws_cancel_service(l_ServiceThread._pContext);
  
  // Destroy the websocket context.
  lws_context_destroy(l_ServiceThread._pContext);
  l_ServiceThread._pContext = nullptr;
  
  // Free string buffer.
  free(l_Config._pMessage);
  l_Config._pMessage = nullptr;
  
  // Done.
  return EXIT_SUCCESS;
}

/************************************************************************/
/* Functions implementation                                             */
/************************************************************************/

//=======================================================================
lws* ConnectClient  ( lws_context* ipContext
                    , const int iPort
                    , const char* ipProtocolName)
//=======================================================================
{
  if( (nullptr == ipContext) || (nullptr == ipProtocolName) ) return nullptr;

  struct lws_client_connect_info ccinfo = {0};
  ccinfo.context = ipContext;
  ccinfo.address = "localhost";
  ccinfo.port = iPort;
  ccinfo.path = "/chat";
  ccinfo.host = lws_canonical_hostname(ipContext);
  ccinfo.origin = "origin";
  ccinfo.protocol = ipProtocolName;
  
  // Done.
  return lws_client_connect_via_info(&ccinfo);
}

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
  
  // Server configuration structure.
  ClientConfigStruct* l_pConfig = l_pServiceThread->_pConfig;
  if( (nullptr == l_pConfig)
  ||  (nullptr == l_pConfig->_pProtocols)
  ||  (nullptr == l_pServiceThread->_pContext) )
#ifdef _WINDOWS_SOURCE
    return l_Ret;
#else   // _WINDOWS_SOURCE
    return nullptr;
#endif  // _WINDOWS_SOURCE
  
  // Infinite loop.
  do
  {
    if( nullptr == l_pConfig->_pClient )
    {
      // Reconnect to the server, if need be.
      l_pConfig->_pClient = ConnectClient ( l_pServiceThread->_pContext
                                          , l_pConfig->_Port
                                          , l_pConfig->_pProtocols[0].name);
    }
    
    // Server connection is on.
    if( nullptr != l_pConfig->_pClient )
    {
      // Lock.
      /* TODO */
      
      // libwebsockets is single thread, cycle through events here.
      if( 0 != (l_pServiceThread->_Ret = lws_service(l_pServiceThread->_pContext, /* timeout_ms = */ 50)) )
      {
        // Log.
        fprintf(stdout, "Service broken (code:%d).\n", l_pServiceThread->_Ret);
        fflush(stdout);
      }
      
      // Unlock.
      /* TODO */
    }
    // Wait for break.
    //fprintf(stdout, "%ld breath (%d)\n", time(nullptr), l_pServiceThread->_Ret);
    //fflush(stdout);
  }
  while( 0 == l_pServiceThread->_Break );

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
  ClientConfigStruct* l_pConfig = (ClientConfigStruct *)lws_context_user(l_pContext);
  if( nullptr == l_pConfig ) return l_Ret;

  // Do the right thing.
  switch( reason )
  {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      {
        // Log.
        fprintf(stdout, "Client connection established\n");
        fflush(stdout);
      }
      break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
      {
        // Format response.
        char l_aResponse[BUFSIZ];
        memcpy(l_aResponse, in, len);
        l_aResponse[len] = '\0';
        
        // Print message back.
        fprintf(stdout, "%s\t\t(len:%zd)\n", l_aResponse, len);
        fflush(stdout);
      }
      break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
      {
        if( nullptr == l_pConfig->_pMessage ) break;
        
        // Lock.
        /* TODO */
     
        // Message length.
        size_t l_Len = strlen(l_pConfig->_pMessage) + 1;
       
        unsigned char* buf = nullptr;
        
        do
        {
          // Create message envelope.
          if( nullptr == (buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + l_Len + LWS_SEND_BUFFER_POST_PADDING)) ) break;
          
          // Copy letter.
          memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], l_pConfig->_pMessage, l_Len);
          
          // Write to server.
          lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], l_Len, LWS_WRITE_TEXT);
        }
        while( false );
        
        // Delete string buffer.
        free(l_pConfig->_pMessage);
        l_pConfig->_pMessage = nullptr;
        
        // Delete string buffer.
        free(buf);
        buf = nullptr;
        
        // Unlock.
        /* TODO */
      }
      break;
    case LWS_CALLBACK_WSI_DESTROY:
      {
        // Log.
        fprintf(stdout, "LWS_CALLBACK_WSI_DESTROY.\n");
        fflush(stdout);
      }
      break;
    case LWS_CALLBACK_CLIENT_CLOSED :
      {
        // Reset client.
        l_pConfig->_pClient = nullptr;
        
        // Log.
        fprintf(stdout, "Client closed.\n");
        fflush(stdout);
      }
      break;
    case LWS_CALLBACK_CLOSED:
      {
        // Reset client.
        l_pConfig->_pClient = nullptr;
        
        // Log.
        fprintf(stdout, "Connection closed\n");
        fflush(stdout);
      }
      break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      {
        // Reset client.
        l_pConfig->_pClient = nullptr;
        
        // Log.
        fprintf(stdout, "Client connection error.\n");
        fflush(stdout);
      }
      break;
    default:
      break;
  }
  
  // Done.
  return l_Ret;
}

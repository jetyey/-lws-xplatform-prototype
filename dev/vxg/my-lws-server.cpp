/************************************************************************/
/* Global include                                                       */
/************************************************************************/

/************************************************************************/
/* Local include                                                        */
/************************************************************************/
#include "my-lws-common.h"
#include "cJSON.h"

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#define CLIENT_CONNECTED_MAX              10
#define CLIENT_UPLOAD_HISTORY_MAX         10

/************************************************************************/
/* Classes declaration                                                  */
/************************************************************************/

#ifdef __cpluplus
extern "C" {
#endif  // __cpluplus

/** @struct UploadFileStruct */
struct UploadFileStruct
{
  /// Upload status.
  lws_spa_fileupload_states _Status;

  /// File path.
  char* _pFilePath;
};

/** @struct ClientStruct */
struct ClientStruct
{
  /// Method type.
  int _HTTPMethod;

  /// Protocol type.
  ProtocolType_t _Protocol;

  /// WebSocket client.
  lws* _pWsClient;

  /// Upload history size.
  size_t _lUploadFilePathHistory;

  /// Upload history.
  UploadFileStruct** _ppUploadFilePathHistory;

  /// Array of parameters.
  const char * const * _ppParam;

  /// Size of array of parameter.
  size_t _lParam;

  /// Upload text field.
  char* _pUploadTextField;

  /// GET text field.
  char* _pGetRequest;
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

  /// Upload history max size.
  int _lClientUploadFileHistoryMax;

  /// Client array.
  ClientStruct** _ppClient;

  /// Ressource path.
  char* _pResourcePath;

  /// Upload path.
  char* l_pDestination;

  /// Message buffer length.
  size_t _lMessageBuffer;

  /// Message buffer.
  unsigned char* _pMessageBuffer;

  /// Connected client count.
  size_t _ClientConnectedCount;
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

  /// Step.
  int _Step;

  /// POST arguments parser handle.
  struct lws_spa* spa;

  /// The filename of the uploaded file/
  char filename[128];

  /// The amount of bytes uploaded.
  size_t file_length;
  size_t file_length_total;

  /// fd on file being saved.
  int fd;

  /// Current upload file structure.
  UploadFileStruct* _UploadFileCurr;

  /// Configuration object.
  ServerConfigStruct* _pConfig;

  /// Current WebSocket client.
  ClientStruct* _pWsClientCurr;
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
extern "C" int fProtocolHTTP  ( struct lws* wsi
                              , enum lws_callback_reasons reason
                              , void* user
                              , void* in
                              , size_t len);
//=======================================================================

//=======================================================================
extern "C" int fProtocolChat  ( struct lws* wsi
                              , enum lws_callback_reasons reason
                              , void* user
                              , void* in
                              , size_t len);
//=======================================================================

//=======================================================================
extern "C" int fProtocolUpload  ( struct lws* wsi
                                , enum lws_callback_reasons reason
                                , void* user
                                , void* in
                                , size_t len);
//=======================================================================

/************************************************************************/
/************************************************************************/

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
ClientStruct* ClientConnectAdd(ServerConfigStruct& irConfig, const lws* ipWebSocket, const ProtocolType_t iProtocol);
//=======================================================================

//=======================================================================
ClientStruct* ClientConnectFind(ServerConfigStruct& irConfig, const lws* ipWebSocket);
//=======================================================================

//=======================================================================
size_t ClientConnectCount(ServerConfigStruct& irConfig, const ProtocolType_t iProtocol = PROTOCOL_UNKNOWN);
//=======================================================================

//=======================================================================
int ClientConnectRemove(ServerConfigStruct& irConfig, const lws* ipWebSocket);
//=======================================================================

/************************************************************************/
/************************************************************************/

//=======================================================================
int ClientConnectMessageSendToOne(ServerConfigStruct& irConfig, const char* ipMessage, const size_t ilMessage, lws* ipWebSocket);
//=======================================================================

//=======================================================================
int ClientConnectMessageDispatch(ServerConfigStruct& irConfig, const char* ipMessage, const size_t ilMessage, const lws* ipWebSocket = nullptr, const ProtocolType_t iProtocol = PROTOCOL_UNKNOWN);
//=======================================================================

/************************************************************************/
/************************************************************************/

//=======================================================================
int WriteResponseJSON(ServerConfigStruct& irConfig, lws* ipWebSocket, const int iHTTPStatus, const char* ipContent, const size_t iContentLength);
//=======================================================================

//=======================================================================
int WriteResponseHTTP(ServerConfigStruct& irConfig, lws* ipWebSocket, const int iHTTPStatus, const char* ipContent, const size_t iContentLength);
//=======================================================================

/************************************************************************/
/* Programm core                                                        */
/************************************************************************/

//=======================================================================
int main( int argc, char *argv[] )
//=======================================================================
{
#ifdef _MACOSX_SOURCE
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("Current working dir: %s\n", cwd);
  } else {
    perror("getcwd() error");
    
    // Oops.
    return EXIT_FAILURE;
  }
#endif  // _MACOSX_SOURCE
  
  // Print version.
  fprintf(stdout, "LWS version: %s\n", lws_get_library_version());
  fflush(stdout);

  // Array of protocols.
  struct lws_protocols l_aProtocols[] =
  {
    { "protocol-HTTP"     , fProtocolHTTP   , sizeof(ConnectionDataStruct)  , 0 }
  , { "protocol-chat"     , fProtocolChat   , sizeof(ConnectionDataStruct)  , 0 }
  , { "protocol-upload"   , fProtocolUpload , sizeof(ConnectionDataStruct)  , 0 }
    /**/
  , { nullptr             , nullptr         ,  0                            , 0 }
  };
  
  ServerConfigStruct l_Config;
  
  // Reset.
  memset(&l_Config, 0, sizeof(l_Config));
  
  // Initialize configuration.
  l_Config._Port = 8000;
  l_Config._lClientMax = CLIENT_CONNECTED_MAX;
  l_Config._lClientUploadFileHistoryMax = CLIENT_UPLOAD_HISTORY_MAX;

  // Set ressource path.
#ifdef _WINDOWS_SOURCE
  l_Config._pResourcePath = strdup("C:\\Users\\vxg\\Documents\\GitHub\\lws-xplatform-prototype\\rsc\\my-lws-server");
  l_Config.l_pDestination = strdup("E:\\temp");
#else   // _WINDOWS_SOURCE
  l_Config._pResourcePath = strdup("/Users/sebastien/Documents/git/lws-xplatform-prototype/rsc/my-lws-server");
  l_Config.l_pDestination = strdup("/tmp");
#endif  // _WINDOWS_SOURCE
  
  // Create array of clients.
  if( nullptr == (l_Config._ppClient = (ClientStruct **)malloc((l_Config._lClientMax * sizeof(ClientStruct *)))) ) return EXIT_FAILURE;
  
  ClientStruct* l_pClient = nullptr;
  UploadFileStruct* l_pUploadFile = nullptr;

  // Reset all items.
  for( int c = 0; c < l_Config._lClientMax; c++ )
  {
    // Dummy reset.
    l_Config._ppClient[c] = nullptr;

    // Create client object.
    if( nullptr != (l_pClient = (ClientStruct *)malloc(sizeof(ClientStruct))) )
    {
      // Reset.
      memset(l_pClient, 0x00, sizeof(ClientStruct));

      // Create upload file history buffer.
      if( nullptr != (l_pClient->_ppUploadFilePathHistory = (UploadFileStruct **)malloc((sizeof(UploadFileStruct *) * (l_Config._lClientUploadFileHistoryMax)))) )
      {
        // Reset.
        for( int f = 0; f < l_Config._lClientUploadFileHistoryMax; f++ )
        {
          if( nullptr != (l_pUploadFile = (UploadFileStruct *)malloc(sizeof(UploadFileStruct))) )
          {
            // Reset.
            memset(l_pUploadFile, 0x00, sizeof(UploadFileStruct));

            l_pClient->_ppUploadFilePathHistory[f] = l_pUploadFile;
          }
        }
      }

      // Set.
      l_Config._ppClient[c] = l_pClient;
    }
  }

  // Context information structure.
  struct lws_context_creation_info info;
  
  // Reset.
  memset(&info, 0, sizeof(info));
  
  char l_aCertCrt[BUFSIZ];
  snprintf(l_aCertCrt, sizeof(l_aCertCrt), "%s" slash "server.cert", l_Config._pResourcePath);

  char l_aCertKey[BUFSIZ];
  snprintf(l_aCertKey, sizeof(l_aCertKey), "%s" slash "server.key", l_Config._pResourcePath);

  // Initialize context information.
  info.port = l_Config._Port;
  info.protocols = l_aProtocols;
  info.user = &l_Config;
  info.gid = -1;
  info.uid = -1;
  // Use secured layer.
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.ssl_cert_filepath = l_aCertCrt;
  info.ssl_private_key_filepath = l_aCertKey;

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
 
#ifdef _MACOSX_SOURCE
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
#ifdef _MACOSX_SOURCE
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
 
#ifdef _MACOSX_SOURCE
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

  if( nullptr != l_Config._ppClient )
  {
    UploadFileStruct* l_pUploadFile = nullptr;

    // Reset all items.
    for( int c = 0; c < l_Config._lClientMax; c++ )
    {
      l_pClient = l_Config._ppClient[c];

      // Walk history.
      for( int f = 0; f < l_Config._lClientUploadFileHistoryMax; f++ )
      {
        // File path.
        if( nullptr != (l_pUploadFile = l_pClient->_ppUploadFilePathHistory[f]) )
        { 
          // Delete string buffer.
          free(l_pUploadFile->_pFilePath);
          l_pUploadFile->_pFilePath = nullptr;
        }

        // Delete object.
        free(l_pUploadFile);
        l_pUploadFile = nullptr;
      }

      // Delete string buffer.
      free(l_pClient->_pUploadTextField);
      l_pClient->_pUploadTextField = nullptr;

      // Delete string buffer.
      free(l_pClient->_pGetRequest);
      l_pClient->_pGetRequest = nullptr;

      // Delete client object.
      free(l_pClient);
      l_pClient = nullptr;
    }

    // Delete client array.
    free(l_Config._ppClient);
    l_Config._ppClient = nullptr;
  }
  // Delete string buffer.
  free(l_Config._pResourcePath);
  l_Config._pResourcePath = nullptr;

  // Delete string buffer.
  free(l_Config._pMessageBuffer);
  l_Config._pMessageBuffer = nullptr;

  // Done.
  return EXIT_SUCCESS;
}

/************************************************************************/
/* Functions implementation                                             */
/************************************************************************/

//=======================================================================
ClientStruct* ClientConnectAdd(ServerConfigStruct& irConfig, const lws* ipWebSocket, const ProtocolType_t iProtocol)
//=======================================================================
{
  ClientStruct* l_pClientCandidate = nullptr;
  
  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) ) return l_pClientCandidate;
  
  int c = 0;
  
  ClientStruct* l_pClient = nullptr;
  ClientStruct* l_pClientSaved = nullptr;

  // Walk client list.
  for( ; c < irConfig._lClientMax; c++ )
  {
    // Client object.
    if( nullptr == (l_pClient = irConfig._ppClient[c]) ) continue;

    // Existing slot.
    if( (ipWebSocket == l_pClient->_pWsClient) && (iProtocol == l_pClient->_Protocol) )
    {
      // Found.
      l_pClientCandidate = l_pClient;

      // Log.
      fprintf(stdout, "Already connected.\n");
      fflush(stdout);
      
      // Get out.
      break;
    }
    
    // New slot.
    if( (nullptr == l_pClientSaved) && (nullptr == l_pClient->_pWsClient) )
    {
      // Save slot address.
      l_pClientSaved = l_pClient;
    }
  }
  
  // No more clients.
  if( nullptr == l_pClientSaved )
  {
    // Log.
    fprintf(stdout, "Max connection reached.\n");
    fflush(stdout);
  }
  else
  {
    // Set WebSocket client object address.
    l_pClientSaved->_pWsClient = (lws *)ipWebSocket;
     
    // Save protocol type.
    l_pClientSaved->_Protocol = iProtocol;

    UploadFileStruct* l_pUploadFile = nullptr;

    // Walk history.
    for (int f = 0; f < l_pClientSaved->_lUploadFilePathHistory; f++)
    {
      // File path.
      if( nullptr != (l_pUploadFile = l_pClientSaved->_ppUploadFilePathHistory[f]) )
      {
        // Swap.
        l_pClientSaved->_ppUploadFilePathHistory[f] = nullptr;

        // Delete string buffer.
        free(l_pUploadFile->_pFilePath);
        l_pUploadFile->_pFilePath = nullptr;

        // Delete object.
        free(l_pUploadFile);
        l_pUploadFile = nullptr;
      }
    }

    // Reset count.
    l_pClientSaved->_lUploadFilePathHistory = 0;

    // Delete string buffer.
    free(l_pClientSaved->_pUploadTextField);
    l_pClientSaved->_pUploadTextField = nullptr;

    // Delete string buffer.
    free(l_pClientSaved->_pGetRequest);
    l_pClientSaved->_pGetRequest = nullptr;

    // Found.
    l_pClientCandidate = l_pClientSaved;

    // Log.
    fprintf(stdout, "New client connected.\n");
    fflush(stdout);
  }
  
  // Done.
  return l_pClientCandidate;
}

//=======================================================================
ClientStruct* ClientConnectFind(ServerConfigStruct& irConfig, const lws* ipWebSocket)
//=======================================================================
{
  ClientStruct* l_pClientCandidate = nullptr;

  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) ) return l_pClientCandidate;

  ClientStruct* l_pClient = nullptr;

  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Client object.
    if( nullptr == (l_pClient = irConfig._ppClient[c]) ) continue;

    // Client found.
    if( ipWebSocket == l_pClient->_pWsClient )
    {
      // Found.
      l_pClientCandidate = l_pClient;

      // Get out.
      break;
    }
  }

  // Done.
  return l_pClientCandidate;
}

//=======================================================================
size_t ClientConnectCount(ServerConfigStruct& irConfig, const ProtocolType_t iProtocol/*= PROTOCOL_UNKNOWN*/)
//=======================================================================
{
  size_t l_Count = 0;

  ClientStruct* l_pClient = nullptr;

  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Client object.
    if( nullptr == (l_pClient = irConfig._ppClient[c]) ) continue;

    // Update count.
    if( nullptr != l_pClient->_pWsClient )
    {
      // All protocols
      if( PROTOCOL_UNKNOWN == iProtocol ) l_Count++;
      // Selected protocol.
      else if( 0 != (iProtocol & l_pClient->_Protocol) ) l_Count++;
    }
  }

  // Done.
  return l_Count;
}

//=======================================================================
int ClientConnectRemove(ServerConfigStruct& irConfig, const lws* ipWebSocket)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == ipWebSocket) || (nullptr == irConfig._ppClient) ) return l_Ret;
  
  ClientStruct* l_pClient = nullptr;

  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Client object.
    if( nullptr == (l_pClient = irConfig._ppClient[c]) ) continue;

    // Client found.
    if( ipWebSocket == l_pClient->_pWsClient )
    {
      // Set WebSocket client object address.
      l_pClient->_pWsClient = nullptr;

      UploadFileStruct* l_pUploadFile = nullptr;

      // Walk history.
      for( int f = 0; f < irConfig._lClientUploadFileHistoryMax; f++ )
      {
        // File path.
        if( nullptr != (l_pUploadFile = l_pClient->_ppUploadFilePathHistory[f]) )
        {
          // Swap.
          l_pClient->_ppUploadFilePathHistory[f] = nullptr;

          // Delete string buffer.
          free(l_pUploadFile->_pFilePath);
          l_pUploadFile->_pFilePath = nullptr;

          // Delete object.
          free(l_pUploadFile);
          l_pUploadFile = nullptr;
        }
      }

      // Reset count.
      l_pClient->_lUploadFilePathHistory = 0;
      
      // Delete string buffer.
      free(l_pClient->_pUploadTextField);
      l_pClient->_pUploadTextField = nullptr;

      // Delete string buffer.
      free(l_pClient->_pGetRequest);
      l_pClient->_pGetRequest = nullptr;

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
int ClientConnectMessageSendToOne(ServerConfigStruct& irConfig, const char* ipMessage, const size_t ilMessage, lws* ipWebSocket)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == irConfig._ppClient) || (nullptr == ipMessage) || (nullptr == ipWebSocket) ) return l_Ret;
  
  size_t l_IncomingLen = LWS_SEND_BUFFER_PRE_PADDING + ilMessage /* LWS_SEND_BUFFER_POST_PADDING is deprecated. */;

  // Upgrade buffer size.
  if( nullptr == UpgradeBuffer((char **)&irConfig._pMessageBuffer, &irConfig._lMessageBuffer, l_IncomingLen) ) return l_Ret;

  // Copy letter.
  memcpy(&irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], ipMessage, ilMessage);

  // Send response.
  size_t l_LenSent = lws_write(ipWebSocket, &irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], ilMessage, LWS_WRITE_TEXT);

  if( ilMessage <= l_LenSent )
  {
    // Ok.
    l_Ret = 0;
  }
  else
  {
    // Oops.
    l_Ret = -1;
      
    // Log.
    fprintf(stdout, "Sent size error : '%zd' (sent:%zd) --> '%s'\n", ilMessage, l_LenSent, ipMessage);
    fflush(stdout);
  }

  // Done.
  return l_Ret;
}

//=======================================================================
int ClientConnectMessageDispatch(ServerConfigStruct& irConfig, const char* ipMessage, const size_t ilMessage, const lws* ipWebSocket/* = nullptr*/, const ProtocolType_t iProtocol/* = PROTOCOL_UNKNOWN*/)
//=======================================================================
{
  int l_Ret = -1;
  
  if( (nullptr == irConfig._ppClient) || (nullptr == ipMessage) ) return l_Ret;
  
  // Reset.
  l_Ret = 0;

  ClientStruct* l_pClient = nullptr;

  // Walk client list.
  for( int c = 0; c < irConfig._lClientMax; c++ )
  {
    // Skip myself.
    if( (nullptr == (l_pClient = irConfig._ppClient[c]))
    ||  (nullptr == l_pClient->_pWsClient)
    ||  (ipWebSocket == l_pClient->_pWsClient) ) continue;
    
    // Select protocol.
    if( (PROTOCOL_UNKNOWN != iProtocol) && (0 == (iProtocol & l_pClient->_Protocol)) ) continue;

    // Do send to client.
    if( 0 != ClientConnectMessageSendToOne(irConfig, ipMessage, ilMessage, l_pClient->_pWsClient) )
    {
      // Oops.
      l_Ret = -1;
    }
  }

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
static const char * get_mimetype(const char *file)
//=======================================================================
{
  int n = strlen(file);
  
  if (n < 5)
    return NULL;
  
  if (!strcmp(&file[n - 4], ".ico"))
    return "image/x-icon";
  
  if (!strcmp(&file[n - 4], ".png"))
    return "image/png";
  
  if (!strcmp(&file[n - 5], ".html"))
    return "text/html";
  
  return NULL;
}

//=======================================================================
static void dump_handshake_info(struct lws *wsi)
//=======================================================================
{
  int n = 0;
  char buf[256];
  const unsigned char *c;
  
  do {
    c = lws_token_to_string((lws_token_indexes)n);
    if (!c) {
      n++;
      continue;
    }
    
    if (!lws_hdr_total_length(wsi, (lws_token_indexes)n)) {
      n++;
      continue;
    }
    
    lws_hdr_copy(wsi, buf, sizeof buf, (lws_token_indexes)n);
    
    fprintf(stdout, "    %s = %s\n", (char *)c, buf);
    n++;
  }
  while( c );
}

//=======================================================================
static int file_upload_cb ( void *data
                          , const char *name
                          , const char *filename
                          , char *buf
                          , int len
                          , enum lws_spa_fileupload_states state)
  //=======================================================================
{
	struct ConnectionDataStruct *l_pData = (struct ConnectionDataStruct *)data;
  if( (nullptr == l_pData) || (nullptr == l_pData->_pConfig) ) return -1;

  // Do the right thing.
	switch( state )
  {
	case LWS_UFS_OPEN:
    {
      // Filter POST file name. 
      if( (nullptr == name) || (0 != strcmp(name, "post_file")) ) return 1;

		  // Copy the provided filename.
		  lws_strncpy(l_pData->filename, filename, sizeof(l_pData->filename) - 1);

		  /* remove any scary things like .. */
		  //lws_filename_purify_inplace(l_pData->filename);

      char l_aPath[BUFSIZ];
      
      // Build file full path.
      snprintf(l_aPath, sizeof(l_aPath), "%s" slash "%s", l_pData->_pConfig->l_pDestination, l_pData->filename);

		  // Open file.
#ifdef _WINDOWS_SOURCE
      _sopen_s(&l_pData->fd, l_aPath, (_O_WRONLY | _O_CREAT | _O_BINARY), _SH_DENYNO, (_S_IREAD | _S_IWRITE));
#else   // _WINDOWS_SOURCE
      l_pData->fd = open(l_aPath, (O_CREAT | O_TRUNC | O_RDWR), 0600);
#endif  // _WINDOWS_SOURCE
		  if( -1 == l_pData->fd )
      {
			  fprintf(stdout, "Failed to open output file %s\n", l_pData->filename);
        fflush(stdout);

        // Get out.
			  return 1;
		  }

      // Uploaded file history.
      if( (nullptr != l_pData->_pWsClientCurr) && (nullptr != l_pData->_pWsClientCurr->_ppUploadFilePathHistory) && (l_pData->_pWsClientCurr->_lUploadFilePathHistory < l_pData->_pConfig->_lClientUploadFileHistoryMax) )
      {
        // New file slot.
        if( nullptr == (l_pData->_UploadFileCurr = l_pData->_pWsClientCurr->_ppUploadFilePathHistory[l_pData->_pWsClientCurr->_lUploadFilePathHistory]) )
        {
          if( nullptr != (l_pData->_UploadFileCurr = (UploadFileStruct *)malloc(sizeof(UploadFileStruct))) )
          {
            // Set new slot.
            l_pData->_pWsClientCurr->_ppUploadFilePathHistory[l_pData->_pWsClientCurr->_lUploadFilePathHistory] = l_pData->_UploadFileCurr;

            // Reset.
            memset(l_pData->_UploadFileCurr, 0x00, sizeof(UploadFileStruct));
          }
        }


        if( nullptr != l_pData->_UploadFileCurr )
        {
          // Update file counter.
          l_pData->_pWsClientCurr->_lUploadFilePathHistory++;

          // Status.
          l_pData->_UploadFileCurr->_Status = state;

          // Add file.
          l_pData->_UploadFileCurr->_pFilePath = strdup(l_aPath);
        }
        else
        {
          if( 0 < l_pData->fd )
          {
            // Close file.
		        close(l_pData->fd);
            l_pData->fd = -1;
          }
        }
      }
    }
		break;
	case LWS_UFS_FINAL_CONTENT:
	case LWS_UFS_CONTENT:
    {
		  if( 0 < len )
      {
			  int n;

        l_pData->file_length += len;

			  n = write(l_pData->fd, (const void *)buf, (unsigned int)len);
			  if (n < len) {
          fprintf(stdout, "Problem writing file %d\n", errno);
			  }
		  }

      // File upload completed.
		  if( LWS_UFS_FINAL_CONTENT == state )
      {
		    fprintf(stdout, "%s: upload done, written %lld to %s\n", __func__,  l_pData->file_length, l_pData->filename);

        if( 0 < l_pData->fd )
        {
          // Close file.
		      close(l_pData->fd);
          l_pData->fd = -1;
        }

        // Reset size.
        l_pData->file_length = 0;
        l_pData->file_length_total = 0;
      }
    }
		break;
	case LWS_UFS_CLOSE:
    {
      if( 0 < l_pData->fd )
      {
        // Close file.
		    close(l_pData->fd);
        l_pData->fd = -1;
      }

      // Reset size.
      l_pData->file_length = 0;
      l_pData->file_length_total = 0;

      // Current upload file structure.
      if( nullptr != l_pData->_UploadFileCurr ) l_pData->_UploadFileCurr->_Status = state;
    }
		break;
	}

	return 0;
}

//=======================================================================
extern "C" int fProtocolHTTP  ( struct lws* wsi
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
  if( (nullptr == l_pConfig) || (nullptr == l_pConfig->_pResourcePath) ) return l_Ret;

  // Per session data.
  ConnectionDataStruct* l_pData = (ConnectionDataStruct *)(user);
  if( nullptr != l_pData ) l_pData->_pConfig = l_pConfig;

  char l_aResourceFilePath[BUFSIZ];

  // Do the right thing.
  switch ( reason )
  {
  case LWS_CALLBACK_CLOSED_HTTP:
    {
      // Log.
      fprintf(stdout, "fProtocolHTTP - Connection closed (%p).\n", wsi);
      fflush(stdout);

      // Remove client.
      ClientConnectRemove(*l_pConfig, wsi);

      // Get notified as soon as.
      lws_callback_on_writable(wsi);
    }
    break;
  case LWS_CALLBACK_HTTP:
    {
      // Log.
      fprintf(stdout, "fProtocolHTTP - Protocol initialization (%p).\n", wsi);
      fflush(stdout);

      const char* l_pIn = (const char *)in;
      if( nullptr == l_pIn ) break;
     
      fprintf(stdout, "* LWS_CALLBACK_HTTP ********************************************************\n");

      dump_handshake_info(wsi);
      
      int n = 0;
      char buf[BUFSIZ];
      while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_URI_ARGS, n) > 0)
      { lwsl_notice("URI Arg %d: %s\n", ++n, buf); }
      
      // Root path.
      if( 0 == strcmp(l_pIn, "/") )
      {
        snprintf(l_aResourceFilePath, sizeof(l_aResourceFilePath), "%s%s", l_pConfig->_pResourcePath, "/index.html");
        int n = lws_serve_http_file(wsi, l_aResourceFilePath, "text/html", NULL, 0);
        if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
          l_Ret = -1; /* error or can't reuse connection: close the socket */

        //if (lws_http_transaction_completed(wsi))
        //  return -1;
      }
      else
      {
        // GET method.
        if( 0 < lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI) )
        {
          // Start step.
          l_pData->_Step = 1;

          // Form url.
          if( 0 == strcasecmp((const char *)in, "/get_form") )
          {
            if( nullptr != l_pData )
            {
              // Upgrade buffer size.
              if( nullptr == UpgradeBuffer(&l_pData->_pBuffer, &l_pData->_lBuffer, (LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE)) ) break;

              // First time, add client.
              if( (nullptr == l_pData->_pWsClientCurr) && (nullptr == (l_pData->_pWsClientCurr = ClientConnectAdd(*l_pConfig, wsi, PROTOCOL_HTTP))) )
              {
                // Oops.
                break;
              }

              // Method.
              l_pData->_pWsClientCurr->_HTTPMethod = LWSHUMETH_GET;

              const char* val;
              const char* key;

              // Just dump the decoded things to the log.
              if( nullptr == (val = lws_get_urlarg_by_name(wsi, (key = "get_name"), l_pData->_pBuffer, l_pData->_lBuffer)) )
              {
                if( nullptr != (val = lws_get_urlarg_by_name(wsi, (key = "get_message"), l_pData->_pBuffer, l_pData->_lBuffer)) )
                {
                  ///////////////////////////////////////////////////////////////////////////////////////////////
                  ClientStruct* l_pClient = nullptr;

                  // Walk client list.
                  for( int c = 0; c < l_pConfig->_lClientMax; c++ )
                  {
                    // Skip myself.
                    if( (nullptr == (l_pClient = l_pConfig->_ppClient[c]))
                    ||  (nullptr == l_pClient->_pWsClient) ) continue;

                    // Select protocol.
                    //if ((PROTOCOL_UNKNOWN != PROTOCOL_CHAT) && (0 == (PROTOCOL_CHAT & l_pClient->_Protocol))) continue;
                    if( PROTOCOL_CHAT == l_pClient->_Protocol ) continue;

                    if( wsi == l_pClient->_pWsClient )
                    {
                      // Save text field string.
                      l_pClient->_pGetRequest = strdup(val);

                      // Get notified as soon as.
                      lws_callback_on_writable(l_pClient->_pWsClient);

                      // Get out.
                      break;
                    }
                  }

                  // Start step.
                  l_pData->_Step = 0;

                  // Get out.
                  break;
                  ///////////////////////////////////////////////////////////////////////////////////////////////
                }
                else
                {
                  fprintf(stdout, "%s: undefined\n", key);
                }
              }
              else
              {
                ///////////////////////////////////////////////////////////////////////////////////////////////
                ClientStruct* l_pClient = nullptr;

                // Walk client list.
                for (int c = 0; c < l_pConfig->_lClientMax; c++)
                {
                  // Skip myself.
                  if ((nullptr == (l_pClient = l_pConfig->_ppClient[c]))
                    || (nullptr == l_pClient->_pWsClient)
                    /*|| (wsi == l_pClient->_pWsClient)*/) continue;

                  // Select protocol.
                  //if ((PROTOCOL_UNKNOWN != PROTOCOL_CHAT) && (0 == (PROTOCOL_CHAT & l_pClient->_Protocol))) continue;
                  if (PROTOCOL_CHAT == l_pClient->_Protocol) continue;

                  // Save text field string.
                  l_pClient->_pUploadTextField = strdup(val);

                  // Get notified as soon as.
                  lws_callback_on_writable(l_pClient->_pWsClient);
                }
                ///////////////////////////////////////////////////////////////////////////////////////////////
              }
            }

            // Root page.
            snprintf(l_aResourceFilePath, sizeof(l_aResourceFilePath), "%s%s", l_pConfig->_pResourcePath, "/page_get.html");
            int n = lws_serve_http_file(wsi, l_aResourceFilePath, "text/html", NULL, 0);
            if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
              l_Ret = -1; /* error or can't reuse connection: close the socket */
          }
          // Favorite icon.
          else if( 0 == strcasecmp(l_pIn, "/favicon.ico") )
          {
            // Root page.
            snprintf(l_aResourceFilePath, sizeof(l_aResourceFilePath), "%s%s", l_pConfig->_pResourcePath, l_pIn);
            int n = lws_serve_http_file(wsi, l_aResourceFilePath, "image/x-icon", NULL, 0);
            if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
              l_Ret = -1; /* error or can't reuse connection: close the socket */
          }
          else
          {
            /* default to 404-ing the URL if not mounted */
            l_Ret = lws_callback_http_dummy(wsi, reason, user, in, len);
          }

          // Reset.
          l_pData->_Step = 0;
        }
        // POST method.
        else if( 0 < lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI) )
        {
          // Form url.
          if( 0 == strcmp((const char *)in, "/post_form") )
          {
            if( nullptr != l_pData )
            {
              // First time, add client.
              if( (nullptr == l_pData->_pWsClientCurr) && (nullptr == (l_pData->_pWsClientCurr = ClientConnectAdd(*l_pConfig, wsi, PROTOCOL_HTTP))) )
              {
                // Oops.
                break;
              }

              // Start step.
              l_pData->_Step = 1;

              // Method.
              l_pData->_pWsClientCurr->_HTTPMethod = LWSHUMETH_POST;
            }
          }
          else
          {
            /* default to 404-ing the URL if not mounted */
            l_Ret = lws_callback_http_dummy(wsi, reason, user, in, len);
          }
        }
        else
        {
          /* default to 404-ing the URL if not mounted */
          l_Ret = lws_callback_http_dummy(wsi, reason, user, in, len);
        }
      }

      fprintf(stdout, "****************************************************************************\n");
    }
    break;
  case LWS_CALLBACK_HTTP_BODY:
    {
      if( nullptr == l_pData ) break;

      fprintf(stdout, "* LWS_CALLBACK_HTTP_BODY ***************************************************\n");

      // Create the POST argument parser if not already existing;
      if ( nullptr == l_pData->spa )
      {
        char ctype[128];
        int ctlen;

        // Content type.
        ctlen = lws_hdr_copy(wsi, ctype, sizeof(ctype) - 1, WSI_TOKEN_HTTP_CONTENT_TYPE);
        if (ctlen > 0) fprintf(stdout, "HTTP_BODY: content type: %s (%d)\n", ctype, ctlen);
        fflush(stdout);

        // Content length.
        ctlen = lws_hdr_copy(wsi, ctype, sizeof(ctype) - 1, WSI_TOKEN_HTTP_CONTENT_LENGTH);
        if (ctlen > 0) fprintf(stdout, "HTTP_BODY: content length: %s (%d)\n", ctype, ctlen);
        fflush(stdout);

        UploadFileStruct* l_pUploadFile = nullptr;

        // Walk history.
        for( int f = 0; f < l_pConfig->_lClientUploadFileHistoryMax; f++ )
        {
          // File path.
          if( nullptr != (l_pUploadFile = l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f]) )
          { 
            // Delete string buffer.
            free(l_pUploadFile->_pFilePath);
            l_pUploadFile->_pFilePath = nullptr;

            // Reset.
            memset(l_pUploadFile, 0x00, sizeof(UploadFileStruct));
          }
        }

        // Reset current count.
        l_pData->_pWsClientCurr->_lUploadFilePathHistory = 0;

        static const char * const l_aParamNames[] =
        { "post_name" };
        l_pData->_pWsClientCurr->_ppParam = l_aParamNames;
        l_pData->_pWsClientCurr->_lParam = 1;

        // Create POST processor
        if(  nullptr == (l_pData->spa = lws_spa_create  ( wsi
                                                        , l_pData->_pWsClientCurr->_ppParam
                                                        , l_pData->_pWsClientCurr->_lParam
                                                        , 1024
                                                        , file_upload_cb
                                                        , l_pData)) )
        { return -1; }
      }

      if( nullptr != l_pData->spa )
      {
        // let it parse the POST data.
        if (0 != lws_spa_process(l_pData->spa, (const char *)in, (int)len))
          return -1;
      }

      fprintf(stdout, "****************************************************************************\n");
    }
    break;
  case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
    {
      if( nullptr == l_pData ) break;

      if( nullptr != l_pData->spa )
      {
        // Destroy POST processor.
        if( 0 != lws_spa_destroy(l_pData->spa) )
          return -1;

        // Reset.
        l_pData->spa = nullptr;
      }
    }
    break;
  case LWS_CALLBACK_HTTP_BODY_COMPLETION:
    {
      if( nullptr == l_pData ) break;

      fprintf(stdout, "* LWS_CALLBACK_HTTP_BODY_COMPLETION ****************************************\n");

      // Finalize POST.
      lws_spa_finalize(l_pData->spa);

      if( nullptr != l_pData->_pWsClientCurr )
      {
        // Delete string buffer.
        free(l_pData->_pWsClientCurr->_pUploadTextField);
        l_pData->_pWsClientCurr->_pUploadTextField = nullptr;

        // Get first field.
        const char* l_pUploadTextField = lws_spa_get_string(l_pData->spa, 0/* First field. */);

        if( nullptr != l_pUploadTextField )
        {
          // Save text field string.
          l_pData->_pWsClientCurr->_pUploadTextField = strdup(l_pUploadTextField);
        }
      }

      // Destroy POST processor.
      if( 0 != lws_spa_destroy(l_pData->spa) )
        return -1;

      // Reset.
      l_pData->spa = nullptr;

      // Reset.
      l_pData->_Step = 0;

      // Get notified as soon as.
      lws_callback_on_writable(wsi);

      fprintf(stdout, "****************************************************************************\n");
    }
    break;
  case LWS_CALLBACK_HTTP_WRITEABLE:
    {
      if( (nullptr == l_pData) || (nullptr == l_pData->_pWsClientCurr) ) break;

      fprintf(stdout, "* LWS_CALLBACK_HTTP_WRITEABLE **********************************************\n");

      size_t l_LenResponse = 0;
      char l_aResponse[BUFSIZ];

      // Do the rigth thing
      switch( l_pData->_pWsClientCurr->_HTTPMethod )
      {
      case LWSHUMETH_GET:
        {
          // Text field string not set yet.
          if( nullptr == l_pData->_pWsClientCurr->_pGetRequest )
          {
            // Write response back.
            l_Ret = WriteResponseJSON(*l_pConfig, wsi, HTTP_STATUS_OK, "()", 2);
          }
          // Text field string set, format response.
          else if( 0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"message\": \"%s (I am %d)\"}", l_pData->_pWsClientCurr->_pGetRequest, l_pData->_Id)) )
          {
            fprintf(stdout, "-----------------------------\n");
            fprintf(stdout, "Text ====> '%s'\n", l_aResponse);

            // Write response back.
            l_Ret = WriteResponseJSON(*l_pConfig, wsi, HTTP_STATUS_OK, l_aResponse, l_LenResponse);
          }
          // Error.
          else
          {
            // Ko.
            l_Ret = lws_return_http_status(wsi, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error");
          }

          // Delete string buffer.
          free(l_pData->_pWsClientCurr->_pGetRequest);
          l_pData->_pWsClientCurr->_pGetRequest = nullptr;
        }
        break;
      case LWSHUMETH_POST:
        {
          // Still posting.
          if( 0 != l_pData->_Step )  break;

          // Text field string not set yet.
          if( nullptr == l_pData->_pWsClientCurr->_pUploadTextField )
          { snprintf(l_aResponse, sizeof(l_aResponse), "??"); }
          else
          {
            // Format response.
            if( 0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"message\": \"%s (I am %d)\"}", l_pData->_pWsClientCurr->_pUploadTextField, l_pData->_Id)) )
            {
              fprintf(stdout, "-----------------------------\n");
              fprintf(stdout, "Text ====> '%s'\n", l_pData->_pWsClientCurr->_pUploadTextField);

              // Dispatch response.
              //ClientConnectMessageDispatch(*l_pConfig, l_aResponse, l_LenResponse, wsi, PROTOCOL_CHAT);
            }
            else
            { snprintf(l_aResponse, sizeof(l_aResponse), "()"); }

            // Delete string buffer.
            free(l_pData->_pWsClientCurr->_pUploadTextField);
            l_pData->_pWsClientCurr->_pUploadTextField = nullptr;
          }


          // Uploaded file history.
          if( nullptr != l_pData->_pWsClientCurr->_ppUploadFilePathHistory )
          {
            size_t l_Len = 0;
            const char* l_pFilePath = nullptr;
            UploadFileStruct* l_pUploadFileStruct = nullptr;

            fprintf(stdout, "-----------------------------\n");
            for (int f = 0; f < l_pData->_pWsClientCurr->_lUploadFilePathHistory; f++)
            {
              if( nullptr == (l_pUploadFileStruct = l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f]) ) continue;

              // File path.
              if( nullptr == (l_pFilePath = l_pUploadFileStruct->_pFilePath) ) continue;

              // Append path.
              strncat_s(l_aResponse, sizeof(l_aResponse), "\n", (l_Len = 1));
              l_LenResponse += l_Len;
              strncat_s(l_aResponse, sizeof(l_aResponse), l_pFilePath, (l_Len = strlen(l_pFilePath)));
              l_LenResponse += l_Len;

              // Add file.
              fprintf(stdout, "File ====> '%s'\n", l_pFilePath);

              // Dispatch response.
              //ClientConnectMessageSendToOne(*l_pConfig, l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f], (strlen(l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f]) + 1), l_pData->_pWsClientCurr->_pWsClient);
            }
            fprintf(stdout, "-----------------------------\n");
          }

          //p += lws_snprintf((char *)p, end - p, "<html>"
          //  "<head><meta charset=utf-8 "
          //  "http-equiv=\"Content-Language\" "
          //  "content=\"en\"/></head><body>"
          //  "<img src=\"/libwebsockets.org-logo.svg\">"
          //  "<br>Dynamic content for '%s' from mountpoint."
          //  "<br>Time: %s<br><br>"
          //  "</body></html>", pss->path, ctime(&t));


          // Write response back.
          l_Ret = WriteResponseHTTP(*l_pConfig, wsi, HTTP_STATUS_OK, l_aResponse, l_LenResponse);
        }
        break;
      default:
        {}
      }

      fprintf(stdout, "****************************************************************************\n");
    }
    break;
  default:
    {
    }
  }
  
  // Done.
  return l_Ret;
}

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

  // Per session data.
  ConnectionDataStruct* l_pData = (ConnectionDataStruct *)(user);
  if( nullptr != l_pData ) l_pData->_pConfig = l_pConfig;

  // Do the right thing.
  switch ( reason )
  {
  case LWS_CALLBACK_PROTOCOL_INIT:
    {
      // Log.
      fprintf(stdout, "fProtocolChat - Protocol initialization (%p).\n", wsi);
      fflush(stdout);
    }
    break;
  case LWS_CALLBACK_ESTABLISHED:
    {
      if( nullptr == l_pData ) break;

      // First time, add client.
      if( (nullptr == l_pData->_pWsClientCurr) && (nullptr == (l_pData->_pWsClientCurr = ClientConnectAdd(*l_pConfig, wsi, PROTOCOL_CHAT))) )
      {
        // Oops.
        break;
      }

      char l_aUri[64];

      // Parse request URI.
      lws_hdr_copy(wsi, l_aUri, sizeof(l_aUri), WSI_TOKEN_GET_URI);

      // Log.
      fprintf(stdout, "fProtocolChat - Uri: '%s'.\n", l_aUri);
      fflush(stdout);

      // Log.
      fprintf(stdout, "fProtocolChat - Connection established (wsi:%p, l_pData->_pWsClientCurr:%p).\n", wsi, l_pData->_pWsClientCurr);
      fflush(stdout);
      
      // Get client addresses.
      if( nullptr == lws_get_peer_simple(wsi, l_pData->_aClientAddr, sizeof(l_pData->_aClientAddr)) )
      {
        // Log.
        fprintf(stdout, "fProtocolChat - Network connection error from '%s'.\n", l_pData->_aClientAddr);
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
      fprintf(stdout, "fProtocolChat - LWS_CALLBACK_WSI_DESTROY.\n");
      fflush(stdout);
    }
    break;
  case LWS_CALLBACK_CLOSED:
    {
      if( nullptr == l_pData ) break;
   
      // Remove client.
      ClientConnectRemove(*l_pConfig, wsi);
      
      // Log.
      fprintf(stdout, "fProtocolChat - Connection closed (%p, %zd).\n", wsi, l_pData->_lBuffer);
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
      fprintf(stdout, "fProtocolChat - Network connect from %s (%s).\n", l_aClientName, l_aClientIP);
      fflush(stdout);
      
      // if non-zero returned here, connection is killed.
      //return -1;
    }
    break;
  case LWS_CALLBACK_RECEIVE:
    {
      if( nullptr == l_pData ) break;
      
      // Upgrade buffer size.
      if( nullptr == UpgradeBuffer(&l_pData->_pBuffer, &l_pData->_lBuffer, len) ) break;
      
      // Copy incoming data.
      memcpy(l_pData->_pBuffer, in, len);

      // Null-terminated string is expected.
      if( '\0' != l_pData->_pBuffer[len] ) l_pData->_pBuffer[len] = '\0';

      // Log.
      fprintf(stdout, "fProtocolChat - Client %03d/%03d [%s] - Request '%s' (len:%zd)\n", l_pData->_Id, (int)l_pConfig->_ClientCounter, l_pData->_aClientAddr, l_pData->_pBuffer, l_pData->_lBuffer);
      fflush(stdout);
      
      // Get notified as soon as.
      lws_callback_on_writable(wsi);
    }
    break;
  case LWS_CALLBACK_SERVER_WRITEABLE:
    {
      if( nullptr == l_pData ) break;

      char l_aResponse[BUFSIZ];

      size_t l_LenResponse = 0;

      // Connected client count.
      size_t l_Count = ClientConnectCount(*l_pConfig, PROTOCOL_CHAT);

      // Format response.
      if( 0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"connected_chat\":\"%zd\"}", l_Count)) )
      {
        // Dispatch response.
        ClientConnectMessageSendToOne(*l_pConfig, l_aResponse, l_LenResponse, wsi);
      }

      // Basic message.
      if( (nullptr != l_pData->_pBuffer) && ('\0' != *l_pData->_pBuffer) )
      {
        // Format response.
        if( 0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"message\": \"%s (I am %d)\"}", l_pData->_pBuffer, l_pData->_Id)) )
        {
          // Reset message.
          *l_pData->_pBuffer = '\0';

          // Dispatch response.
          ClientConnectMessageDispatch(*l_pConfig, l_aResponse, l_LenResponse, wsi, PROTOCOL_CHAT);
        }

        // Reset message.
        *l_pData->_pBuffer = '\0';
      }
    }
    break;
  default:
    {
    }
  }
  
  // Done.
  return l_Ret;
}

//=======================================================================
extern "C" int fProtocolUpload  ( struct lws* wsi
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

  // Per session data.
  ConnectionDataStruct* l_pData = (ConnectionDataStruct *)(user);
  if( nullptr != l_pData ) l_pData->_pConfig = l_pConfig;

  // Do the right thing.
  switch( reason )
  {
  case LWS_CALLBACK_PROTOCOL_INIT:
    {
      // Log.
      fprintf(stdout, "fProtocolUpload - Protocol initialization (%p).\n", wsi);
      fflush(stdout);
    }
    break;
  case LWS_CALLBACK_ESTABLISHED:
    {
      if( nullptr == l_pData ) break;

      // First time, add client.
      if( (nullptr == l_pData->_pWsClientCurr) && (nullptr == (l_pData->_pWsClientCurr = ClientConnectAdd(*l_pConfig, wsi, PROTOCOL_UPLOAD))) )
      {
        // Oops.
        break;
      }

      char l_aUri[64];

      // Parse request URI.
      lws_hdr_copy(wsi, l_aUri, sizeof(l_aUri), WSI_TOKEN_GET_URI);

      // Log.
      fprintf(stdout, "fProtocolUpload - Uri: '%s'.\n", l_aUri);
      fflush(stdout);

      // Log.
      fprintf(stdout, "fProtocolUpload - Connection established (%p).\n", wsi);
      fflush(stdout);
      
      // Get client addresses.
      if( nullptr == lws_get_peer_simple(wsi, l_pData->_aClientAddr, sizeof(l_pData->_aClientAddr)) )
      {
        // Log.
        fprintf(stdout, "fProtocolUpload - Network connection error from '%s'.\n", wsi);
        fflush(stdout);
        
        // Close session.
        l_Ret = -1;
        
        // Get out.
        break;
      }
    }
    break;
  case LWS_CALLBACK_CLOSED:
    {
      if( nullptr == l_pData ) break;
   
      // Remove client.
      ClientConnectRemove(*l_pConfig, wsi);
      
      // Log.
      fprintf(stdout, "fProtocolUpload - Connection closed (%p, %zd).\n", wsi, l_pData->_lBuffer);
      fflush(stdout);
      
      // Free buffer string.
      free(l_pData->_pBuffer);
      l_pData->_pBuffer = nullptr;
    }
    break;
  case LWS_CALLBACK_RECEIVE:
    {
      if( nullptr == l_pData ) break;

      // Upgrade buffer size.
      if( nullptr == UpgradeBuffer((char **)&l_pData->_pBuffer, &l_pData->_lBuffer, len) ) break;

      // Copy incoming data.
      memcpy(l_pData->_pBuffer, in, len);

      // Null-terminated string is expected.
      if( '\0' != l_pData->_pBuffer[len] ) l_pData->_pBuffer[len] = '\0';

      // Decode as JSON.
      cJSON * l_pJSON = cJSON_Parse(l_pData->_pBuffer);
      if( (0 >= l_pData->fd) && (nullptr != l_pJSON) )
      {
        const cJSON* l_pValueFileName = nullptr;
        const cJSON* l_pValueName = nullptr;
        const cJSON* l_pValueSize = nullptr;

        // Get name and file name value.
        if( (cJSON_IsString((l_pValueName     = cJSON_GetObjectItemCaseSensitive(l_pJSON, "name"))))      && (nullptr != l_pValueName->valuestring)
        &&  (cJSON_IsString((l_pValueFileName = cJSON_GetObjectItemCaseSensitive(l_pJSON, "filename"))))  && (nullptr != l_pValueFileName->valuestring) )
        {
          // Begin.
          if( (cJSON_IsString((l_pValueSize = cJSON_GetObjectItemCaseSensitive(l_pJSON, "size")))) && (nullptr != l_pValueSize->valuestring) )
          {
            UploadFileStruct* l_pUploadFile = nullptr;

            // Walk history.
            for( int f = 0; f < l_pConfig->_lClientUploadFileHistoryMax; f++ )
            {
              // File path.
              if( nullptr != (l_pUploadFile = l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f]) )
              { 
                // Delete string buffer.
                free(l_pUploadFile->_pFilePath);
                l_pUploadFile->_pFilePath = nullptr;

                // Reset.
                memset(l_pUploadFile, 0x00, sizeof(UploadFileStruct));
              }
              else
              {
                if( nullptr != (l_pUploadFile = (UploadFileStruct *)malloc(sizeof(UploadFileStruct))) )
                {
                  // Reset.
                  memset(l_pUploadFile, 0x00, sizeof(UploadFileStruct));

                  l_pData->_pWsClientCurr->_ppUploadFilePathHistory[f] = l_pUploadFile;
                }
              }
            }

            // Reset current count.
            l_pData->_pWsClientCurr->_lUploadFilePathHistory = 0;

            // Reset current length.
            l_pData->file_length = 0;

            // Set total length.
            l_pData->file_length_total = atoll(l_pValueSize->valuestring);

            // Start step.
            l_pData->_Step = 1;

            // Open.
            l_Ret = file_upload_cb  ( l_pData
                                    , l_pValueName->valuestring
                                    , l_pValueFileName->valuestring
                                    , nullptr
                                    , 0
                                    , LWS_UFS_OPEN);

            // Get notified as soon as.
            lws_callback_on_writable(wsi);
          }
        }
      }
      else if( l_pData->file_length <= l_pData->file_length_total )
      {
        // Content.
        if( 0 == (l_Ret = file_upload_cb  ( l_pData
                                          , nullptr
                                          , nullptr
                                          , (char *)in
                                          , len
                                          , LWS_UFS_CONTENT)) && (l_pData->file_length == l_pData->file_length_total) )
        {
          // Final content.
          l_Ret = file_upload_cb  ( l_pData
                                  , nullptr
                                  , nullptr
                                  , nullptr
                                  , (l_pData->file_length_total - l_pData->file_length)
                                  , LWS_UFS_FINAL_CONTENT);

          // Close.
          l_Ret = file_upload_cb  ( l_pData
                                  , nullptr
                                  , nullptr
                                  , nullptr
                                  , 0
                                  , LWS_UFS_CLOSE);

          // Reset.
          l_pData->_Step = 0;
        }
        else
        {
          fprintf(stdout, "oops 1\n");
          fflush(stdout);
        }

        // Get notified as soon as.
        lws_callback_on_writable(wsi);
      }
      else
      {
        fprintf(stdout, "oops 2\n");
        fflush(stdout);
      }

      // Delete JSON parser.
      cJSON_Delete(l_pJSON);
    }
    break;
  case LWS_CALLBACK_SERVER_WRITEABLE:
    {
      if( nullptr == l_pData ) break;

      // Still posting.
      //if( 0 != l_pData->_Step )  break;

      char l_aResponse[BUFSIZ];

      size_t l_LenResponse = 0;

      // Connected client count.
      size_t l_Count = ClientConnectCount(*l_pConfig, PROTOCOL_UPLOAD);

      // Format response.
      if( 0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"connected_upload\":\"%zd\"}", l_Count)) )
      {
        // Dispatch response.
        ClientConnectMessageSendToOne(*l_pConfig, l_aResponse, l_LenResponse, wsi);
      }

      UploadFileStruct* l_pUploadFile = nullptr;

      ClientStruct* l_pClient = nullptr;

      // Reset all items.
      if( nullptr != (l_pClient = l_pData->_pWsClientCurr) )
      {
        // Walk history.
        for( int f = 0; f < l_pConfig->_lClientUploadFileHistoryMax; f++ )
        {
          // File path.
          if( (nullptr != (l_pUploadFile = l_pClient->_ppUploadFilePathHistory[f]))
          &&  (nullptr != l_pUploadFile->_pFilePath)
          &&  (LWS_UFS_CLOSE == l_pUploadFile->_Status) )
          {
            // Dummy escape.
            const char * l_pFileName = strrchr(l_pUploadFile->_pFilePath, slash_char);
            if( nullptr != l_pFileName )
            {
              // Format response.
              if (0 < (l_LenResponse = snprintf(l_aResponse, sizeof(l_aResponse), "{\"message\":\"%s\"}", ++l_pFileName)))
              {
                // Dispatch response.
                ClientConnectMessageSendToOne(*l_pConfig, l_aResponse, l_LenResponse, wsi);
              }
            }

            // Reset.
            l_pUploadFile->_Status = LWS_UFS_CONTENT;
          }
        }
      }
    }
    break;
  default:
    {
    }
  }

  // Done.
  return l_Ret;
}

/************************************************************************/
/************************************************************************/

//=======================================================================
static int WriteResponseHeader(ServerConfigStruct& irConfig, lws* ipWebSocket, const int iHTTPStatus, const char* ipContentType, const size_t iContentLength)
//=======================================================================
{
  int l_Ret = -1;

  if( (nullptr == ipWebSocket) || (nullptr == ipContentType) ) return l_Ret;

  do
  {
    unsigned char buffer[LWS_RECOMMENDED_MIN_HEADER_SPACE + LWS_PRE];
    unsigned char* const start = buffer + LWS_PRE;
    unsigned char* const end = buffer + sizeof(buffer);
    unsigned char* p = start;

    // Build header.
    if( 0 != lws_add_http_header_status(ipWebSocket, iHTTPStatus, &p, end) ) break;
    if( 0 != lws_add_http_header_by_token(ipWebSocket, WSI_TOKEN_HTTP_CONTENT_TYPE, (unsigned char *)ipContentType, strlen(ipContentType), &p, end) ) break;
    if( 0 != lws_add_http_header_content_length(ipWebSocket, iContentLength, &p, end) ) break;

    // Write header.
    //if( 0 != lws_finalize_http_header(ipWebSocket, &p, end) ) break;
    //if (0 > lws_write(ipWebSocket, start, lws_ptr_diff(p, start), LWS_WRITE_HTTP_HEADERS)) break;
    if( 0 > lws_finalize_write_http_header(ipWebSocket, start,  &p, end) ) break;

    // Ok.
    l_Ret = 0;
  }
  while( false );

  // Done.
  return l_Ret;
}

//=======================================================================
int WriteResponseHTTP(ServerConfigStruct& irConfig, lws* ipWebSocket, const int iHTTPStatus, const char* ipContent, const size_t iContentLength)
//=======================================================================
{
  int l_Ret = -1;

  if( (nullptr == ipWebSocket) || (nullptr == ipContent) ) return l_Ret;

  do
  {
    // Write header.
    if( 0 != WriteResponseHeader(irConfig, ipWebSocket, iHTTPStatus, "text/html", iContentLength) ) break;

    size_t l_Len = 0;

    if( nullptr == ipContent )
    {
      // Finish HTTP write?
      /* TODO */

      // Ok.
      l_Ret = 0;
    }
    else
    {
      // Content expected size.
      l_Len = LWS_SEND_BUFFER_PRE_PADDING + iContentLength /* LWS_SEND_BUFFER_POST_PADDING is deprecated. */;

      // Upgrade buffer size.
      if( nullptr == UpgradeBuffer((char **)&irConfig._pMessageBuffer, &irConfig._lMessageBuffer, l_Len) ) break;

      // Copy letter.
      memcpy(&irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], ipContent, iContentLength);

      // Do send content response.
      if( 0 > lws_write(ipWebSocket, &irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], iContentLength, LWS_WRITE_HTTP_FINAL) ) break;

      // Ok.
      l_Ret = 0;
    }
  }
  while( false );

  // Done.
  return l_Ret;
}

//=======================================================================
int WriteResponseJSON(ServerConfigStruct& irConfig, lws* ipWebSocket, const int iHTTPStatus, const char* ipContent, const size_t iContentLength)
//=======================================================================
{
  int l_Ret = -1;

  if( (nullptr == ipWebSocket) || (nullptr == ipContent) ) return l_Ret;

  do
  {
    // Write header.
    if( 0 != WriteResponseHeader(irConfig, ipWebSocket, iHTTPStatus, "appplication/json", iContentLength) ) break;

    size_t l_Len = 0;

    if( nullptr == ipContent )
    {
      // Finish HTTP write?
      /* TODO */

      // Ok.
      l_Ret = 0;
    }
    else
    {
      // Content expected size.
      l_Len = LWS_SEND_BUFFER_PRE_PADDING + iContentLength /* LWS_SEND_BUFFER_POST_PADDING is deprecated. */;

      // Upgrade buffer size.
      if( nullptr == UpgradeBuffer((char **)&irConfig._pMessageBuffer, &irConfig._lMessageBuffer, l_Len) ) break;

      // Copy letter.
      memcpy(&irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], ipContent, iContentLength);

      // Do send content response.
      if( 0 > lws_write(ipWebSocket, &irConfig._pMessageBuffer[LWS_SEND_BUFFER_PRE_PADDING], iContentLength, LWS_WRITE_HTTP_FINAL) ) break;

      // Ok.
      l_Ret = 0;
    }
  }
  while( false );

  // Done.
  return l_Ret;
}

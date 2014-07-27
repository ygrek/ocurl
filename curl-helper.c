/***
 ***  curl-helper.c
 ***
 ***  Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 ***  Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 ***/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <curl/curl.h>

#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/unixsupport.h>
#include <caml/custom.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#pragma message("No config file given.")
#endif

void leave_blocking_section(void);
void enter_blocking_section(void);

#define Val_none Val_int(0)

static __inline value
Val_some( value v )
{
    CAMLparam1( v );
    CAMLlocal1( some );
    some = caml_alloc(1, 0);
    Store_field( some, 0, v );
    CAMLreturn( some );
}

static value Val_pair(value v1, value v2)
{
  CAMLparam2(v1,v2);
  CAMLlocal1(pair);
  pair = caml_alloc_small(2,0);
  Field(pair,0) = v1;
  Field(pair,1) = v2;
  CAMLreturn(pair);
}

static value Val_cons(value list, value v) { return Val_pair(v,list); }

typedef struct Connection Connection;
typedef struct ConnectionList ConnectionList;

#define Connection_val(v) (*(Connection**)Data_custom_val(v))

enum OcamlValues
{
    OcamlWriteCallback,
    OcamlReadCallback,
    OcamlErrorBuffer,
    OcamlPostFields,
    OcamlHTTPHeader,
    OcamlHTTPPost,
    OcamlQuote,
    OcamlPostQuote,
    OcamlHeaderCallback,
    OcamlProgressCallback,
    OcamlDebugCallback,
    OcamlHTTP200Aliases,
    OcamlIOCTLCallback,
    OcamlSeekFunctionCallback,
    OcamlOpenSocketFunctionCallback,

    OcamlURL,
    OcamlProxy,
    OcamlUserPWD,
    OcamlProxyUserPWD,
    OcamlRange,
    OcamlReferer,
    OcamlUserAgent,
    OcamlFTPPort,
    OcamlCookie,
    OcamlHTTPPostStrings,
    OcamlSSLCert,
    OcamlSSLCertType,
    OcamlSSLCertPasswd,
    OcamlSSLKey,
    OcamlSSLKeyType,
    OcamlSSLKeyPasswd,
    OcamlSSLEngine,
    OcamlCookieFile,
    OcamlCustomRequest,
    OcamlInterface,
    OcamlCAInfo,
    OcamlCAPath,
    OcamlRandomFile,
    OcamlEGDSocket,
    OcamlCookieJar,
    OcamlSSLCipherList,
    OcamlPrivate,
    OcamlNETRCFile,
    OcamlFTPAccount,
    OcamlCookieList,
    OcamlFTPAlternativeToUser,
    OcamlSSHPublicKeyFile,
    OcamlSSHPrivateKeyFile,
    OcamlSSHHostPublicKeyMD5,
    OcamlCopyPostFields,

    /* Not used, last for size */
    OcamlValuesSize
};

struct Connection
{
    CURL *connection;
    Connection *next;
    Connection *prev;

    value ocamlValues;

    size_t refcount; /* number of references to this structure */

    char *url;
    char *proxy;
    char *userPwd;
    char *proxyUserPwd;
    char *range;
    char *errorBuffer;
    char *postFields;
    int postFieldSize;
    char *referer;
    char *userAgent;
    char *ftpPort;
    char *cookie;
    struct curl_slist *httpHeader;
    struct curl_httppost *httpPostFirst;
    struct curl_httppost *httpPostLast;
    struct curl_slist *httpPostStrings;
    struct curl_slist *resolve;
    char *sslCert;
    char *sslCertType;
    char *sslCertPasswd;
    char *sslKey;
    char *sslKeyType;
    char *sslKeyPasswd;
    char *sslEngine;
    struct curl_slist *quote;
    struct curl_slist *postQuote;
    char *cookieFile;
    char *customRequest;
    char *interface_; /* `interface` gives problems on windows */
    char *caInfo;
    char *caPath;
    char *randomFile;
    char *egdSocket;
    char *cookieJar;
    char *sslCipherList;
    char *private;
    struct curl_slist *http200Aliases;
    char *netrcFile;
    char *ftpaccount;
    char *cookielist;
    char *ftpAlternativeToUser;
    char *sshPublicKeyFile;
    char *sshPrivateKeyFile;
    char *sshHostPublicKeyMD5;
    char *copyPostFields;
    char *dns_servers;
};

struct ConnectionList
{
    Connection *head;
    Connection *tail;
};

static ConnectionList connectionList = {NULL, NULL};

typedef struct CURLErrorMapping CURLErrorMapping;

struct CURLErrorMapping
{
    char *name;
    CURLcode error;
};

CURLErrorMapping errorMap[] =
{
#if HAVE_DECL_CURLE_UNSUPPORTED_PROTOCOL
    {"CURLE_UNSUPPORTED_PROTOCOL", CURLE_UNSUPPORTED_PROTOCOL},
#else
    {"CURLE_UNSUPPORTED_PROTOCOL", -1},
#endif
#if HAVE_DECL_CURLE_FAILED_INIT
    {"CURLE_FAILED_INIT", CURLE_FAILED_INIT},
#else
    {"CURLE_FAILED_INIT", -1},
#endif
#if HAVE_DECL_CURLE_URL_MALFORMAT
    {"CURLE_URL_MALFORMAT", CURLE_URL_MALFORMAT},
#else
    {"CURLE_URL_MALFORMAT", -1},
#endif
#if HAVE_DECL_CURLE_URL_MALFORMAT_USER
    {"CURLE_URL_MALFORMAT_USER", CURLE_URL_MALFORMAT_USER},
#else
    {"CURLE_URL_MALFORMAT_USER", -1},
#endif
#if HAVE_DECL_CURLE_COULDNT_RESOLVE_PROXY
    {"CURLE_COULDNT_RESOLVE_PROXY", CURLE_COULDNT_RESOLVE_PROXY},
#else
    {"CURLE_COULDNT_RESOLVE_PROXY", -1},
#endif
#if HAVE_DECL_CURLE_COULDNT_RESOLVE_HOST
    {"CURLE_COULDNT_RESOLVE_HOST", CURLE_COULDNT_RESOLVE_HOST},
#else
    {"CURLE_COULDNT_RESOLVE_HOST", -1},
#endif
#if HAVE_DECL_CURLE_COULDNT_CONNECT
    {"CURLE_COULDNT_CONNECT", CURLE_COULDNT_CONNECT},
#else
    {"CURLE_COULDNT_CONNECT", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WEIRD_SERVER_REPLY
    {"CURLE_FTP_WEIRD_SERVER_REPLY", CURLE_FTP_WEIRD_SERVER_REPLY},
#else
    {"CURLE_FTP_WEIRD_SERVER_REPLY", -1},
#endif
#if HAVE_DECL_CURLE_FTP_ACCESS_DENIED
    {"CURLE_FTP_ACCESS_DENIED", CURLE_FTP_ACCESS_DENIED},
#else
    {"CURLE_FTP_ACCESS_DENIED", -1},
#endif
#if HAVE_DECL_CURLE_FTP_USER_PASSWORD_INCORRECT
    {"CURLE_FTP_USER_PASSWORD_INCORRECT", CURLE_FTP_USER_PASSWORD_INCORRECT},
#else
    {"CURLE_FTP_USER_PASSWORD_INCORRECT", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WEIRD_PASS_REPLY
    {"CURLE_FTP_WEIRD_PASS_REPLY", CURLE_FTP_WEIRD_PASS_REPLY},
#else
    {"CURLE_FTP_WEIRD_PASS_REPLY", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WEIRD_USER_REPLY
    {"CURLE_FTP_WEIRD_USER_REPLY", CURLE_FTP_WEIRD_USER_REPLY},
#else
    {"CURLE_FTP_WEIRD_USER_REPLY", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WEIRD_PASV_REPLY
    {"CURLE_FTP_WEIRD_PASV_REPLY", CURLE_FTP_WEIRD_PASV_REPLY},
#else
    {"CURLE_FTP_WEIRD_PASV_REPLY", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WEIRD_227_FORMAT
    {"CURLE_FTP_WEIRD_227_FORMAT", CURLE_FTP_WEIRD_227_FORMAT},
#else
    {"CURLE_FTP_WEIRD_227_FORMAT", -1},
#endif
#if HAVE_DECL_CURLE_FTP_CANT_GET_HOST
    {"CURLE_FTP_CANT_GET_HOST", CURLE_FTP_CANT_GET_HOST},
#else
    {"CURLE_FTP_CANT_GET_HOST", -1},
#endif
#if HAVE_DECL_CURLE_FTP_CANT_RECONNECT
    {"CURLE_FTP_CANT_RECONNECT", CURLE_FTP_CANT_RECONNECT},
#else
    {"CURLE_FTP_CANT_RECONNECT", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_SET_BINARY
    {"CURLE_FTP_COULDNT_SET_BINARY", CURLE_FTP_COULDNT_SET_BINARY},
#else
    {"CURLE_FTP_COULDNT_SET_BINARY", -1},
#endif
#if HAVE_DECL_CURLE_PARTIAL_FILE
    {"CURLE_PARTIAL_FILE", CURLE_PARTIAL_FILE},
#else
    {"CURLE_PARTIAL_FILE", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_RETR_FILE
    {"CURLE_FTP_COULDNT_RETR_FILE", CURLE_FTP_COULDNT_RETR_FILE},
#else
    {"CURLE_FTP_COULDNT_RETR_FILE", -1},
#endif
#if HAVE_DECL_CURLE_FTP_WRITE_ERROR
    {"CURLE_FTP_WRITE_ERROR", CURLE_FTP_WRITE_ERROR},
#else
    {"CURLE_FTP_WRITE_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_FTP_QUOTE_ERROR
    {"CURLE_FTP_QUOTE_ERROR", CURLE_FTP_QUOTE_ERROR},
#else
    {"CURLE_FTP_QUOTE_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_HTTP_NOT_FOUND
    {"CURLE_HTTP_NOT_FOUND", CURLE_HTTP_NOT_FOUND},
#else
    {"CURLE_HTTP_NOT_FOUND", -1},
#endif
#if HAVE_DECL_CURLE_WRITE_ERROR
    {"CURLE_WRITE_ERROR", CURLE_WRITE_ERROR},
#else
    {"CURLE_WRITE_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_MALFORMAT_USER
    {"CURLE_MALFORMAT_USER", CURLE_MALFORMAT_USER},
#else
    {"CURLE_MALFORMAT_USER", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_STOR_FILE
    {"CURLE_FTP_COULDNT_STOR_FILE", CURLE_FTP_COULDNT_STOR_FILE},
#else
    {"CURLE_FTP_COULDNT_STOR_FILE", -1},
#endif
#if HAVE_DECL_CURLE_READ_ERROR
    {"CURLE_READ_ERROR", CURLE_READ_ERROR},
#else
    {"CURLE_READ_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_OUT_OF_MEMORY
    {"CURLE_OUT_OF_MEMORY", CURLE_OUT_OF_MEMORY},
#else
    {"CURLE_OUT_OF_MEMORY", -1},
#endif
#if HAVE_DECL_CURLE_OPERATION_TIMEOUTED
    {"CURLE_OPERATION_TIMEOUTED", CURLE_OPERATION_TIMEOUTED},
#else
    {"CURLE_OPERATION_TIMEOUTED", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_SET_ASCII
    {"CURLE_FTP_COULDNT_SET_ASCII", CURLE_FTP_COULDNT_SET_ASCII},
#else
    {"CURLE_FTP_COULDNT_SET_ASCII", -1},
#endif
#if HAVE_DECL_CURLE_FTP_PORT_FAILED
    {"CURLE_FTP_PORT_FAILED", CURLE_FTP_PORT_FAILED},
#else
    {"CURLE_FTP_PORT_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_USE_REST
    {"CURLE_FTP_COULDNT_USE_REST", CURLE_FTP_COULDNT_USE_REST},
#else
    {"CURLE_FTP_COULDNT_USE_REST", -1},
#endif
#if HAVE_DECL_CURLE_FTP_COULDNT_GET_SIZE
    {"CURLE_FTP_COULDNT_GET_SIZE", CURLE_FTP_COULDNT_GET_SIZE},
#else
    {"CURLE_FTP_COULDNT_GET_SIZE", -1},
#endif
#if HAVE_DECL_CURLE_HTTP_RANGE_ERROR
    {"CURLE_HTTP_RANGE_ERROR", CURLE_HTTP_RANGE_ERROR},
#else
    {"CURLE_HTTP_RANGE_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_HTTP_POST_ERROR
    {"CURLE_HTTP_POST_ERROR", CURLE_HTTP_POST_ERROR},
#else
    {"CURLE_HTTP_POST_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_SSL_CONNECT_ERROR
    {"CURLE_SSL_CONNECT_ERROR", CURLE_SSL_CONNECT_ERROR},
#else
    {"CURLE_SSL_CONNECT_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_FTP_BAD_DOWNLOAD_RESUME
    {"CURLE_FTP_BAD_DOWNLOAD_RESUME", CURLE_FTP_BAD_DOWNLOAD_RESUME},
#else
    {"CURLE_FTP_BAD_DOWNLOAD_RESUME", -1},
#endif
#if HAVE_DECL_CURLE_FILE_COULDNT_READ_FILE
    {"CURLE_FILE_COULDNT_READ_FILE", CURLE_FILE_COULDNT_READ_FILE},
#else
    {"CURLE_FILE_COULDNT_READ_FILE", -1},
#endif
#if HAVE_DECL_CURLE_LDAP_CANNOT_BIND
    {"CURLE_LDAP_CANNOT_BIND", CURLE_LDAP_CANNOT_BIND},
#else
    {"CURLE_LDAP_CANNOT_BIND", -1},
#endif
#if HAVE_DECL_CURLE_LDAP_SEARCH_FAILED
    {"CURLE_LDAP_SEARCH_FAILED", CURLE_LDAP_SEARCH_FAILED},
#else
    {"CURLE_LDAP_SEARCH_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_LIBRARY_NOT_FOUND
    {"CURLE_LIBRARY_NOT_FOUND", CURLE_LIBRARY_NOT_FOUND},
#else
    {"CURLE_LIBRARY_NOT_FOUND", -1},
#endif
#if HAVE_DECL_CURLE_FUNCTION_NOT_FOUND
    {"CURLE_FUNCTION_NOT_FOUND", CURLE_FUNCTION_NOT_FOUND},
#else
    {"CURLE_FUNCTION_NOT_FOUND", -1},
#endif
#if HAVE_DECL_CURLE_ABORTED_BY_CALLBACK
    {"CURLE_ABORTED_BY_CALLBACK", CURLE_ABORTED_BY_CALLBACK},
#else
    {"CURLE_ABORTED_BY_CALLBACK", -1},
#endif
#if HAVE_DECL_CURLE_BAD_FUNCTION_ARGUMENT
    {"CURLE_BAD_FUNCTION_ARGUMENT", CURLE_BAD_FUNCTION_ARGUMENT},
#else
    {"CURLE_BAD_FUNCTION_ARGUMENT", -1},
#endif
#if HAVE_DECL_CURLE_BAD_CALLING_ORDER
    {"CURLE_BAD_CALLING_ORDER", CURLE_BAD_CALLING_ORDER},
#else
    {"CURLE_BAD_CALLING_ORDER", -1},
#endif
#if HAVE_DECL_CURLE_HTTP_PORT_FAILED
    {"CURLE_HTTP_PORT_FAILED", CURLE_HTTP_PORT_FAILED},
#else
    {"CURLE_HTTP_PORT_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_BAD_PASSWORD_ENTERED
    {"CURLE_BAD_PASSWORD_ENTERED", CURLE_BAD_PASSWORD_ENTERED},
#else
    {"CURLE_BAD_PASSWORD_ENTERED", -1},
#endif
#if HAVE_DECL_CURLE_TOO_MANY_REDIRECTS
    {"CURLE_TOO_MANY_REDIRECTS", CURLE_TOO_MANY_REDIRECTS},
#else
    {"CURLE_TOO_MANY_REDIRECTS", -1},
#endif
#if HAVE_DECL_CURLE_UNKNOWN_TELNET_OPTION
    {"CURLE_UNKNOWN_TELNET_OPTION", CURLE_UNKNOWN_TELNET_OPTION},
#else
    {"CURLE_UNKNOWN_TELNET_OPTION", -1},
#endif
#if HAVE_DECL_CURLE_TELNET_OPTION_SYNTAX
    {"CURLE_TELNET_OPTION_SYNTAX", CURLE_TELNET_OPTION_SYNTAX},
#else
    {"CURLE_TELNET_OPTION_SYNTAX", -1},
#endif
#if HAVE_DECL_CURLE_SSL_PEER_CERTIFICATE
    {"CURLE_SSL_PEER_CERTIFICATE", CURLE_SSL_PEER_CERTIFICATE},
#else
    {"CURLE_SSL_PEER_CERTIFICATE", -1},
#endif
#if HAVE_DECL_CURLE_GOT_NOTHING
    {"CURLE_GOT_NOTHING", CURLE_GOT_NOTHING},
#else
    {"CURLE_GOT_NOTHING", -1},
#endif
#if HAVE_DECL_CURLE_SSL_ENGINE_NOT_FOUND
    {"CURLE_SSL_ENGINE_NOT_FOUND", CURLE_SSL_ENGINE_NOTFOUND},
#else
    {"CURLE_SSL_ENGINE_NOT_FOUND", -1},
#endif
#if HAVE_DECL_CURLE_SSL_ENGINE_SET_FAILED
    {"CURLE_SSL_ENGINE_SET_FAILED", CURLE_SSL_ENGINE_SETFAILED},
#else
    {"CURLE_SSL_ENGINE_SET_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_SEND_ERROR
    {"CURLE_SEND_ERROR", CURLE_SEND_ERROR},
#else
    {"CURLE_SEND_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_RECV_ERROR
    {"CURLE_RECV_ERROR", CURLE_RECV_ERROR},
#else
    {"CURLE_RECV_ERROR", -1},
#endif
#if HAVE_DECL_CURLE_SHARE_IN_USE
    {"CURLE_SHARE_IN_USE", CURLE_SHARE_IN_USE},
#else
    {"CURLE_SHARE_IN_USE", -1},
#endif
#if HAVE_DECL_CURLE_SSL_CERTPROBLEM
    {"CURLE_SSL_CERTPROBLEN", CURLE_SSL_CERTPROBLEM},
#else
    {"CURLE_SSL_CERTPROBLEN", -1},
#endif
#if HAVE_DECL_CURLE_SSL_CIPHER
    {"CURLE_SSL_CIPHER", CURLE_SSL_CIPHER},
#else
    {"CURLE_SSL_CIPHER", -1},
#endif
#if HAVE_DECL_CURLE_SSL_CACERT
    {"CURLE_SSL_CACERT", CURLE_SSL_CACERT},
#else
    {"CURLE_SSL_CACERT", -1},
#endif
#if HAVE_DECL_CURLE_BAD_CONTENT_ENCODING
    {"CURLE_BAD_CONTENT_ENCODING", CURLE_BAD_CONTENT_ENCODING},
#else
    {"CURLE_BAD_CONTENT_ENCODING", -1},
#endif
#if HAVE_DECL_CURLE_LDAP_INVALID_URL
    {"CURLE_LDAP_INVALID_URL", CURLE_LDAP_INVALID_URL},
#else
    {"CURLE_LDAP_INVALID_URL", -1},
#endif
#if HAVE_DECL_CURLE_FILESIZE_EXCEEDED
    {"CURLE_FILESIZE_EXCEEDED", CURLE_FILESIZE_EXCEEDED},
#else
    {"CURLE_FILESIZE_EXCEEDED", -1},
#endif
#if HAVE_DECL_CURLE_FTP_SSL_FAILED
    {"CURLE_FTP_SSL_FAILED", CURLE_FTP_SSL_FAILED},
#else
    {"CURLE_FTP_SSL_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_SEND_FAIL_REWIND
    {"CURLE_SEND_FAIL_REWIND", CURLE_SEND_FAIL_REWIND},
#else
    {"CURLE_SEND_FAIL_REWIND", -1},
#endif
#if HAVE_DECL_CURLE_SSL_ENGINE_INITFAILED
    {"CURLE_SSL_ENGINE_INITFAILED", CURLE_SSL_ENGINE_INITFAILED},
#else
    {"CURLE_SSL_ENGINE_INITFAILED", -1},
#endif
#if HAVE_DECL_CURLE_LOGIN_DENIED
    {"CURLE_LOGIN_DENIED", CURLE_LOGIN_DENIED},
#else
    {"CURLE_LOGIN_DENIED", -1},
#endif
#if HAVE_DECL_CURLE_TFTP_NOTFOUND
    {"CURLE_TFTP_NOTFOUND", CURLE_TFTP_NOTFOUND},
#else
    {"CURLE_TFTP_NOTFOUND", -1},
#endif
#if HAVE_DECL_CURLE_TFTP_PERM
    {"CURLE_TFTP_PERM", CURLE_TFTP_PERM},
#else
    {"CURLE_TFTP_PERM", -1},
#endif
#if HAVE_DECL_CURLE_REMOTE_DISK_FULL
    {"CURLE_REMOTE_DISK_FULL", CURLE_REMOTE_DISK_FULL},
#else
    {"CURLE_REMOTE_DISK_FULL", -1},
#endif
#if HAVE_DECL_CURLE_TFTP_ILLEGAL
    {"CURLE_TFTP_ILLEGAL", CURLE_TFTP_ILLEGAL},
#else
    {"CURLE_TFTP_ILLEGAL", -1},
#endif
#if HAVE_DECL_CURLE_TFTP_UNKNOWNID
    {"CURLE_TFTP_UNKNOWNID", CURLE_TFTP_UNKNOWNID},
#else
    {"CURLE_TFTP_UNKNOWNID", -1},
#endif
#if HAVE_DECL_CURLE_REMOTE_FILE_EXISTS
    {"CURLE_REMOTE_FILE_EXISTS", CURLE_REMOTE_FILE_EXISTS},
#else
    {"CURLE_REMOTE_FILE_EXISTS", -1},
#endif
#if HAVE_DECL_CURLE_TFTP_NOSUCHUSER
    {"CURLE_TFTP_NOSUCHUSER", CURLE_TFTP_NOSUCHUSER},
#else
    {"CURLE_TFTP_NOSUCHUSER", -1},
#endif
#if HAVE_DECL_CURLE_CONV_FAILED
    {"CURLE_CONV_FAILED", CURLE_CONV_FAILED},
#else
    {"CURLE_CONV_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_CONV_REQUIRED
    {"CURLE_CONV_REQUIRED", CURLE_CONV_REQUIRED},
#else
    {"CURLE_CONV_REQUIRED", -1},
#endif
#if HAVE_DECL_CURLE_SSL_CACERT_BADFILE
    {"CURLE_SSL_CACERT_BADFILE", CURLE_SSL_CACERT_BADFILE},
#else
    {"CURLE_SSL_CACERT_BADFILE", -1},
#endif
#if HAVE_DECL_CURLE_REMOTE_FILE_NOT_FOUND
    {"CURLE_REMOTE_FILE_NOT_FOUND", CURLE_REMOTE_FILE_NOT_FOUND},
#else
    {"CURLE_REMOTE_FILE_NOT_FOUND", -1},
#endif
#if HAVE_DECL_CURLE_SSH
    {"CURLE_SSH", CURLE_SSH},
#else
    {"CURLE_SSH", -1},
#endif
#if HAVE_DECL_CURLE_SSL_SHUTDOWN_FAILED
    {"CURLE_SSL_SHUTDOWN_FAILED", CURLE_SSL_SHUTDOWN_FAILED},
#else
    {"CURLE_SSL_SHUTDOWN_FAILED", -1},
#endif
#if HAVE_DECL_CURLE_AGAIN
    {"CURLE_AGAIN", CURLE_AGAIN},
#else
    {"CURLE_AGAIN", -1},
#endif
    {"CURLE_OK", CURLE_OK},
    {NULL, 0}
};

typedef struct CURLOptionMapping CURLOptionMapping;

struct CURLOptionMapping
{
    void (*optionHandler)(Connection *, value);
    char *name;
    CURLoption option;
};

CURLOptionMapping unimplementedOptionMap[] =
{
    {NULL, "CURLOPT_STDERR", CURLOPT_STDERR},
    {NULL, NULL, 0}
};

static void handleWriteFunction(Connection *, value);
static void handleReadFunction(Connection *, value);
static void handleInFileSize(Connection *, value);
static void handleURL(Connection *, value);
static void handleProxy(Connection *, value);
static void handleProxyPort(Connection *, value);
static void handleHTTPProxyTunnel(Connection *, value);
static void handleVerbose(Connection *, value);
static void handleHeader(Connection *, value);
static void handleNoProgress(Connection *, value);
static void handleNoSignal(Connection *, value);
static void handleNoBody(Connection *, value);
static void handleFailOnError(Connection *, value);
static void handleUpload(Connection *, value);
static void handlePost(Connection *, value);
static void handleFTPListOnly(Connection *, value);
static void handleFTPAppend(Connection *, value);
static void handleNETRC(Connection *, value);
static void handleEncoding(Connection *, value);
static void handleFollowLocation(Connection *, value);
static void handleTransferText(Connection *, value);
static void handlePut(Connection *, value);
static void handleUserPwd(Connection *, value);
static void handleProxyUserPwd(Connection *, value);
static void handleRange(Connection *, value);
static void handleErrorBuffer(Connection *, value);
static void handleTimeout(Connection *, value);
static void handlePostFields(Connection *, value);
static void handlePostFieldSize(Connection *, value);
static void handleReferer(Connection *, value);
static void handleUserAgent(Connection *, value);
static void handleFTPPort(Connection *, value);
static void handleLowSpeedLimit(Connection *, value);
static void handleLowSpeedTime(Connection *, value);
static void handleResumeFrom(Connection *, value);
static void handleCookie(Connection *, value);
static void handleHTTPHeader(Connection *, value);
static void handleHTTPPost(Connection *, value);
static void handleSSLCert(Connection *, value);
static void handleSSLCertType(Connection *, value);
static void handleSSLCertPasswd(Connection *, value);
static void handleSSLKey(Connection *, value);
static void handleSSLKeyType(Connection *, value);
static void handleSSLKeyPasswd(Connection *, value);
static void handleSSLEngine(Connection *, value);
static void handleSSLEngineDefault(Connection *, value);
static void handleCRLF(Connection *, value);
static void handleQuote(Connection *, value);
static void handlePostQuote(Connection *, value);
static void handleHeaderFunction(Connection *, value);
static void handleCookieFile(Connection *, value);
static void handleSSLVersion(Connection *, value);
static void handleTimeCondition(Connection *, value);
static void handleTimeValue(Connection *, value);
static void handleCustomRequest(Connection *, value);
static void handleInterface(Connection *, value);
static void handleKRB4Level(Connection *, value);
static void handleProgressFunction(Connection *, value);
static void handleSSLVerifyPeer(Connection *, value);
static void handleCAInfo(Connection *, value);
static void handleCAPath(Connection *, value);
static void handleFileTime(Connection *, value);
static void handleMaxRedirs(Connection *, value);
static void handleMaxConnects(Connection *, value);
static void handleClosePolicy(Connection *, value);
static void handleFreshConnect(Connection *, value);
static void handleForbidReuse(Connection *, value);
static void handleRandomFile(Connection *, value);
static void handleEGDSocket(Connection *, value);
static void handleConnectTimeout(Connection *, value);
static void handleHTTPGet(Connection *, value);
static void handleSSLVerifyHost(Connection *, value);
static void handleCookieJar(Connection *, value);
static void handleSSLCipherList(Connection *, value);
static void handleHTTPVersion(Connection *, value);
static void handleFTPUseEPSV(Connection *, value);
static void handleDNSCacheTimeout(Connection *, value);
static void handleDNSUseGlobalCache(Connection *, value);
static void handleDebugFunction(Connection *, value);
static void handlePrivate(Connection *, value);
static void handleHTTP200Aliases(Connection *, value);
static void handleUnrestrictedAuth(Connection *, value);
static void handleFTPUseEPRT(Connection *, value);
static void handleHTTPAuth(Connection *, value);
static void handleFTPCreateMissingDirs(Connection *, value);
static void handleProxyAuth(Connection *, value);
static void handleFTPResponseTimeout(Connection *, value);
static void handleIPResolve(Connection *, value);
static void handleMaxFileSize(Connection *, value);
static void handleInFileSizeLarge(Connection *, value);
static void handleResumeFromLarge(Connection *, value);
static void handleMaxFileSizeLarge(Connection *, value);
static void handleNETRCFile(Connection *, value);
static void handleFTPSSL(Connection *, value);
static void handlePostFieldSizeLarge(Connection *, value);
static void handleTCPNoDelay(Connection *, value);
static void handleFTPSSLAuth(Connection *, value);
static void handleIOCTLFunction(Connection *, value);
static void handleFTPAccount(Connection *, value);
static void handleCookieList(Connection *, value);
static void handleIgnoreContentLength(Connection *, value);
static void handleFTPSkipPASVIP(Connection *, value);
static void handleFTPFileMethod(Connection *, value);
static void handleLocalPort(Connection *, value);
static void handleLocalPortRange(Connection *, value);
static void handleConnectOnly(Connection *, value);
static void handleMaxSendSpeedLarge(Connection *, value);
static void handleMaxRecvSpeedLarge(Connection *, value);
static void handleFTPAlternativeToUser(Connection *, value);
static void handleSSLSessionIdCache(Connection *, value);
static void handleSSHAuthTypes(Connection *, value);
static void handleSSHPublicKeyFile(Connection *, value);
static void handleSSHPrivateKeyFile(Connection *, value);
static void handleFTPSSLCCC(Connection *, value);
static void handleTimeoutMS(Connection *, value);
static void handleConnectTimeoutMS(Connection *, value);
static void handleHTTPTransferDecoding(Connection *, value);
static void handleHTTPContentDecoding(Connection *, value);
static void handleNewFilePerms(Connection *, value);
static void handleNewDirectoryPerms(Connection *, value);
static void handlePost301(Connection *, value);
static void handleSSHHostPublicKeyMD5(Connection *, value);
static void handleCopyPostFields(Connection *, value);
static void handleProxyTransferMode(Connection *, value);
static void handleSeekFunction(Connection *, value);
static void handleAutoReferer(Connection *, value);
static void handleOpenSocketFunction(Connection *, value);
static void handleProxyType(Connection *, value);
static void handleProtocols(Connection *, value);
static void handleRedirProtocols(Connection *, value);
static void handleResolve(Connection *, value);
static void handleDnsServers(Connection *, value);

CURLOptionMapping implementedOptionMap[] =
{
    {handleWriteFunction, "CURLOPT_WRITEFUNCTION", CURLOPT_WRITEFUNCTION},
    {handleReadFunction, "CURLOPT_READFUNCTION", CURLOPT_READFUNCTION},
    {handleInFileSize, "CURLOPT_INFILESIZE", CURLOPT_INFILESIZE},
    {handleURL, "CURLOPT_URL", CURLOPT_URL},
    {handleProxy, "CURLOPT_PROXY", CURLOPT_PROXY},
    {handleProxyPort, "CURLOPT_PROXYPORT", CURLOPT_PROXYPORT},
    {handleHTTPProxyTunnel, "CURLOPT_HTTPPROXYTUNNEL", CURLOPT_HTTPPROXYTUNNEL},
    {handleVerbose, "CURLOPT_VERBOSE", CURLOPT_VERBOSE},
    {handleHeader, "CURLOPT_HEADER", CURLOPT_HEADER},
    {handleNoProgress, "CURLOPT_NOPROGRESS", CURLOPT_NOPROGRESS},
#if HAVE_DECL_CURLOPT_NOSIGNAL
    {handleNoSignal, "CURLOPT_NOSIGNAL", CURLOPT_NOSIGNAL},
#else
    {handleNoSignal, "CURLOPT_NOSIGNAL", 0},
#endif
    {handleNoBody, "CURLOPT_NOBODY", CURLOPT_NOBODY},
    {handleFailOnError, "CURLOPT_FAILONERROR", CURLOPT_FAILONERROR},
    {handleUpload, "CURLOPT_UPLOAD", CURLOPT_UPLOAD},
    {handlePost, "CURLOPT_POST", CURLOPT_POST},
    {handleFTPListOnly, "CURLOPT_FTPLISTONLY", CURLOPT_FTPLISTONLY},
    {handleFTPAppend, "CURLOPT_FTPAPPEND", CURLOPT_FTPAPPEND},
    {handleNETRC, "CURLOPT_NETRC", CURLOPT_NETRC},
#if HAVE_DECL_CURLOPT_ENCODING
    {handleEncoding, "CURLOPT_ENCODING", CURLOPT_ENCODING},
#else
    {handleEncoding, "CURLOPT_ENCODING", 0},
#endif
    {handleFollowLocation, "CURLOPT_FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION},
    {handleTransferText, "CURLOPT_TRANSFERTEXT", CURLOPT_TRANSFERTEXT},
    {handlePut, "CURLOPT_PUT", CURLOPT_PUT},
    {handleUserPwd, "CURLOPT_USERPWD", CURLOPT_USERPWD},
    {handleProxyUserPwd, "CURLOPT_PROXYUSERPWD", CURLOPT_PROXYUSERPWD},
    {handleRange, "CURLOPT_RANGE", CURLOPT_RANGE},
    {handleErrorBuffer, "CURLOPT_ERRORBUFFER", CURLOPT_ERRORBUFFER},
    {handleTimeout, "CURLOPT_TIMEOUT", CURLOPT_TIMEOUT},
    {handlePostFields, "CURLOPT_POSTFIELDS", CURLOPT_POSTFIELDS},
    {handlePostFieldSize, "CURLOPT_POSTFIELDSIZE", CURLOPT_POSTFIELDSIZE},
    {handleReferer, "CURLOPT_REFERER", CURLOPT_REFERER},
    {handleUserAgent, "CURLOPT_USERAGENT", CURLOPT_USERAGENT},
    {handleFTPPort, "CURLOPT_FTPPORT", CURLOPT_FTPPORT},
    {handleLowSpeedLimit, "CURLOPT_LOW_SPEED_LIMIT", CURLOPT_LOW_SPEED_LIMIT},
    {handleLowSpeedTime, "CURLOPT_LOW_SPEED_TIME", CURLOPT_LOW_SPEED_TIME},
    {handleResumeFrom, "CURLOPT_RESUME_FROM", CURLOPT_RESUME_FROM},
    {handleCookie, "CURLOPT_COOKIE", CURLOPT_COOKIE},
    {handleHTTPHeader, "CURLOPT_HTTPHEADER", CURLOPT_HTTPHEADER},
    {handleHTTPPost, "CURLOPT_HTTPPOST", CURLOPT_HTTPPOST},
    {handleSSLCert, "CURLOPT_SSLCERT", CURLOPT_SSLCERT},
    {handleSSLCertType, "CURLOPT_SSLCERTTYPE", CURLOPT_SSLCERTTYPE},
    {handleSSLCertPasswd, "CURLOPT_SSLCERTPASSWD", CURLOPT_SSLCERTPASSWD},
    {handleSSLKey, "CURLOPT_SSLKEY", CURLOPT_SSLKEY},
    {handleSSLKeyType, "CURLOPT_SSLKEYTYPE", CURLOPT_SSLKEYTYPE},
    {handleSSLKeyPasswd, "CURLOPT_SSLKEYPASSWD", CURLOPT_SSLKEYPASSWD},
    {handleSSLEngine, "CURLOPT_SSLENGINE", CURLOPT_SSLENGINE},
    {handleSSLEngineDefault, "CURLOPT_SSLENGINE_DEFAULT", CURLOPT_SSLENGINE_DEFAULT},
    {handleCRLF, "CURLOPT_CRLF", CURLOPT_CRLF},
    {handleQuote, "CURLOPT_QUOTE", CURLOPT_QUOTE},
    {handlePostQuote, "CURLOPT_POSTQUOTE", CURLOPT_POSTQUOTE},
    {handleHeaderFunction, "CURLOPT_HEADERFUNCTION", CURLOPT_HEADERFUNCTION},
    {handleCookieFile, "CURLOPT_COOKIEFILE", CURLOPT_COOKIEFILE},
    {handleSSLVersion, "CURLOPT_SSLVERSION", CURLOPT_SSLVERSION},
    {handleTimeCondition, "CURLOPT_TIMECONDITION", CURLOPT_TIMECONDITION},
    {handleTimeValue, "CURLOPT_TIMEVALUE", CURLOPT_TIMEVALUE},
    {handleCustomRequest, "CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST},
    {handleInterface, "CURLOPT_INTERFACE", CURLOPT_INTERFACE},
    {handleKRB4Level, "CURLOPT_KRB4LEVEL", CURLOPT_KRB4LEVEL},
    {handleProgressFunction, "CURLOPT_PROGRESSFUNCTION", CURLOPT_PROGRESSFUNCTION},
    {handleSSLVerifyPeer, "CURLOPT_SSLVERIFYPEER", CURLOPT_SSL_VERIFYPEER},
    {handleCAInfo, "CURLOPT_CAINFO", CURLOPT_CAINFO},
    {handleCAPath, "CURLOPT_CAPATH", CURLOPT_CAPATH},
    {handleFileTime, "CURLOPT_FILETIME", CURLOPT_FILETIME},
    {handleMaxRedirs, "CURLOPT_MAXREDIRS", CURLOPT_MAXREDIRS},
    {handleMaxConnects, "CURLOPT_MAXCONNECTS", CURLOPT_MAXCONNECTS},
    {handleClosePolicy, "CURLOPT_CLOSEPOLICY", CURLOPT_CLOSEPOLICY},
    {handleFreshConnect, "CURLOPT_FRESH_CONNECT", CURLOPT_FRESH_CONNECT},
    {handleForbidReuse, "CURLOPT_FORBID_REUSE", CURLOPT_FORBID_REUSE},
    {handleRandomFile, "CURLOPT_RANDOM_FILE", CURLOPT_RANDOM_FILE},
    {handleEGDSocket, "CURLOPT_EGDSOCKET", CURLOPT_EGDSOCKET},
    {handleConnectTimeout, "CURLOPT_CONNECTTIMEOUT", CURLOPT_CONNECTTIMEOUT},
    {handleHTTPGet, "CURLOPT_HTTPGET", CURLOPT_HTTPGET},
    {handleSSLVerifyHost, "CURLOPT_SSL_VERIFYHOST", CURLOPT_SSL_VERIFYHOST},
    {handleCookieJar, "CURLOPT_COOKIEJAR", CURLOPT_COOKIEJAR},
    {handleSSLCipherList, "CURLOPT_SSL_CIPHERLIST", CURLOPT_SSL_CIPHER_LIST},
    {handleHTTPVersion, "CURLOPT_HTTP_VERSION", CURLOPT_HTTP_VERSION},
    {handleFTPUseEPSV, "CURLOPT_FTP_USE_EPSV", CURLOPT_FTP_USE_EPSV},
    {handleDNSCacheTimeout, "CURLOPT_DNS_CACHE_TIMEOUT", CURLOPT_DNS_CACHE_TIMEOUT},
    {handleDNSUseGlobalCache, "CURLOPT_DNS_USE_GLOBAL_CACHE", CURLOPT_DNS_USE_GLOBAL_CACHE},
    {handleDebugFunction, "CURLOPT_DEBUGFUNCTION", CURLOPT_DEBUGFUNCTION},
#if HAVE_DECL_CURLOPT_PRIVATE
    {handlePrivate, "CURLOPT_PRIVATE", CURLOPT_PRIVATE},
#else
    {handlePrivate, "CURLOPT_PRIVATE", 0},
#endif
#if HAVE_DECL_CURLOPT_HTTP200ALIASES
    {handleHTTP200Aliases, "CURLOPT_HTTP200ALIASES", CURLOPT_HTTP200ALIASES},
#else
    {handleHTTP200Aliases, "CURLOPT_HTTP200ALIASES", 0},
#endif
#if HAVE_DECL_CURLOPT_UNRESTRICTED_AUTH
    {handleUnrestrictedAuth, "CURLOPT_UNRESTRICTED_AUTH", CURLOPT_UNRESTRICTED_AUTH},
#else
    {handleUnrestrictedAuth, "CURLOPT_UNRESTRICTED_AUTH", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_USE_EPRT
    {handleFTPUseEPRT, "CURLOPT_FTP_USE_EPRT", CURLOPT_FTP_USE_EPRT},
#else
    {handleFTPUseEPRT, "CURLOPT_FTP_USE_EPRT", 0},
#endif
#if HAVE_DECL_CURLOPT_HTTPAUTH
    {handleHTTPAuth, "CURLOPT_HTTPAUTH", CURLOPT_HTTPAUTH},
#else
    {handleHTTPAuth, "CURLOPT_HTTPAUTH", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_CREATE_MISSING_DIRS
    {handleFTPCreateMissingDirs, "CURLOPT_FTP_CREATE_MISSING_DIRS", CURLOPT_FTP_CREATE_MISSING_DIRS},
#else
    {handleFTPCreateMissingDirs, "CURLOPT_FTP_CREATE_MISSING_DIRS", 0},
#endif
#if HAVE_DECL_CURLOPT_PROXYAUTH
    {handleProxyAuth, "CURLOPT_PROXYAUTH", CURLOPT_PROXYAUTH},
#else
    {handleProxyAuth, "CURLOPT_PROXYAUTH", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_RESPONSE_TIMEOUT
    {handleFTPResponseTimeout, "CURLOPT_FTP_RESPONSE_TIMEOUT", CURLOPT_FTP_RESPONSE_TIMEOUT},
#else
    {handleFTPResponseTimeout, "CURLOPT_FTP_RESPONSE_TIMEOUT", 0},
#endif
#if HAVE_DECL_CURLOPT_IPRESOLVE
    {handleIPResolve, "CURLOPT_IPRESOLVE", CURLOPT_IPRESOLVE},
#else
    {handleIPResolve, "CURLOPT_IPRESOLVE", 0},
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE
    {handleMaxFileSize, "CURLOPT_MAXFILESIZE", CURLOPT_MAXFILESIZE},
#else
    {handleMaxFileSize, "CURLOPT_MAXFILESIZE", 0},
#endif
#if HAVE_DECL_CURLOPT_INFILSIZE_LARGE
    {handleInFileSizeLarge, "CURLOPT_INFILESIZE_LARGE", CURLOPT_INFILESIZE_LARGE},
#else
    {handleInFileSizeLarge, "CURLOPT_INFILESIZE_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_RESUME_FROM_LARGE
    {handleResumeFromLarge, "CURLOPT_RESUME_FROM_LARGE", CURLOPT_RESUME_FROM_LARGE},
#else
    {handleResumeFromLarge, "CURLOPT_RESUME_FROM_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE_LARGE
    {handleMaxFileSizeLarge, "CURLOPT_MAXFILESIZE_LARGE", CURLOPT_MAXFILESIZE_LARGE},
#else
    {handleMaxFileSizeLarge, "CURLOPT_MAXFILESIZE_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_NETRC_FILE
    {handleNETRCFile, "CURLOPT_NETRC_FILE", CURLOPT_NETRC_FILE},
#else
    {handleNETRCFile, "CURLOPT_NETRC_FILE", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL
    {handleFTPSSL, "CURLOPT_FTP_SSL", CURLOPT_FTP_SSL},
#else
    {handleFTPSSL, "CURLOPT_FTP_SSL", 0},
#endif
#if HAVE_DECL_CURLOPT_POSTFIELDSIZE_LARGE
    {handlePostFieldSizeLarge, "CURLOPT_POSTFIELDSIZE_LARGE", CURLOPT_POSTFIELDSIZE_LARGE},
#else
    {handlePostFieldSizeLarge, "CURLOPT_POSTFIELDSIZE_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_TCP_NODELAY
    {handleTCPNoDelay, "CURLOPT_TCP_NODELAY", CURLOPT_TCP_NODELAY},
#else
    {handleTCPNoDelay, "CURLOPT_TCP_NODELAY", 0},
#endif
#if HAVE_DECL_CURLOPT_FTPSSLAUTH
    {handleFTPSSLAuth, "CURLOPT_FTPSSLAUTH", CURLOPT_FTPSSLAUTH},
#else
    {handleFTPSSLAuth, "CURLOPT_FTPSSLAUTH", 0},
#endif
#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
    {handleIOCTLFunction, "CURLOPT_IOCTLFUNCTION", CURLOPT_IOCTLFUNCTION},
#else
    {handleIOCTLFunction, "CURLOPT_IOCTLFUNCTION", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_ACCOUNT
    {handleFTPAccount, "CURLOPT_FTP_ACCOUNT", CURLOPT_FTP_ACCOUNT},
#else
    {handleFTPAccount, "CURLOPT_FTP_ACCOUNT", 0},
#endif
#if HAVE_DECL_CURLOPT_COOKIELIST
    {handleCookieList, "CURLOPT_COOKIELIST", CURLOPT_COOKIELIST},
#else
    {handleCookieList, "CURLOPT_COOKIELIST", 0},
#endif
#if HAVE_DECL_CURLOPT_IGNORE_CONTENT_LENGTH
    {handleIgnoreContentLength, "CURLOPT_IGNORE_CONTENT_LENGTH", CURLOPT_IGNORE_CONTENT_LENGTH},
#else
    {handleIgnoreContentLength, "CURLOPT_IGNORE_CONTENT_LENGTH", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_SKIP_PASV_IP
    {handleFTPSkipPASVIP, "CURLOPT_FTP_SKIP_PASV_IP", CURLOPT_FTP_SKIP_PASV_IP},
#else
    {handleFTPSkipPASVIP, "CURLOPT_FTP_SKIP_PASV_IP", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_FILEMETHOD
    {handleFTPFileMethod, "CURLOPT_FTP_FILEMETHOD", CURLOPT_FTP_FILEMETHOD},
#else
    {handleFTPFileMethod, "CURLOPT_FTP_FILEMETHOD", 0},
#endif
#if HAVE_DECL_CURLOPT_LOCALPORT
    {handleLocalPort, "CURLOPT_LOCALPORT", CURLOPT_LOCALPORT},
#else
    {handleLocalPort, "CURLOPT_LOCALPORT", 0},
#endif
#if HAVE_DECL_CURLOPT_LOCALPORTRANGE
    {handleLocalPortRange, "CURLOPT_LOCALPORTRANGE", CURLOPT_LOCALPORTRANGE},
#else
    {handleLocalPortRange, "CURLOPT_LOCALPORTRANGE", 0},
#endif
#if HAVE_DECL_CURLOPT_CONNECT_ONLY
    {handleConnectOnly, "CURLOPT_CONNECT_ONLY", CURLOPT_CONNECT_ONLY},
#else
    {handleConnectOnly, "CURLOPT_CONNECT_ONLY", 0},
#endif
#if HAVE_DECL_CURLOPT_MAX_SEND_SPEED_LARGE
    {handleMaxSendSpeedLarge, "CURLOPT_MAX_SEND_SPEED_LARGE", CURLOPT_MAX_SEND_SPEED_LARGE},
#else
    {handleMaxSendSpeedLarge, "CURLOPT_MAX_SEND_SPEED_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_MAX_RECV_SPEED_LARGE
    {handleMaxRecvSpeedLarge, "CURLOPT_MAX_RECV_SPEED_LARGE", CURLOPT_MAX_RECV_SPEED_LARGE},
#else
    {handleMaxRecvSpeedLarge, "CURLOPT_MAX_RECV_SPEED_LARGE", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_ALTERNATIVE_TO_USER
    {handleFTPAlternativeToUser, "CURLOPT_FTP_ALTERNATIVE_TO_USER", CURLOPT_FTP_ALTERNATIVE_TO_USER},
#else
    {handleFTPAlternativeToUser, "CURLOPT_FTP_ALTERMATIVE_TO_USER", 0},
#endif
#if HAVE_DECL_CURLOPT_SSL_SESSIONID_CACHE
    {handleSSLSessionIdCache, "CURLOPT_SSL_SESSIONID_CACHE", CURLOPT_SSL_SESSIONID_CACHE},
#else
    {handleSSLSessionIdCache, "CURLOPT_SSL_SESSIONID_CACHE", 0},
#endif
#if HAVE_DECL_CURLOPT_SSH_AUTH_TYPES
    {handleSSHAuthTypes, "CURLOPT_SSH_AUTH_TYPES", CURLOPT_SSH_AUTH_TYPES},
#else
    {handleSSHAuthTypes, "CURLOPT_SSH_AUTH_TYPES", 0},
#endif
#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEYFILE
    {handleSSHPublicKeyFile, "CURLOPT_SSH_PUBLIC_KEYFILE", CURLOPT_SSH_PUBLIC_KEYFILE},
#else
    {handleSSHPublicKeyFile, "CURLOPT_SSH_PUBLIC_KEYFILE", 0},
#endif
#if HAVE_DECL_CURLOPT_SSH_PRIVATE_KEYFILE
    {handleSSHPrivateKeyFile, "CURLOPT_SSH_PRIVATE_KEYFILE", CURLOPT_SSH_PRIVATE_KEYFILE},
#else
    {handleSSHPrivateKeyFile, "CURLOPT_SSH_PRIVATE_KEYFILE", 0},
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL_CCC
    {handleFTPSSLCCC, "CURLOPT_FTP_SSL_CCC", CURLOPT_FTP_SSL_CCC},
#else
    {handleFTPSSLCCC, "CURLOPT_FTP_SSL_CCC", 0},
#endif
#if HAVE_DECL_CURLOPT_TIMEOUT_MS
    {handleTimeoutMS, "CURLOPT_TIMEOUT_MS", CURLOPT_TIMEOUT_MS},
#else
    {handleTimeoutMS, "CURLOPT_TIMEOUT_MS", 0},
#endif
#if HAVE_DECL_CURLOPT_CONNECTTIMEOUT_MS
    {handleConnectTimeoutMS, "CURLOPT_CONNECTTIMEOUT_MS", CURLOPT_CONNECTTIMEOUT_MS},
#else
    {handleConnectTimeoutMS, "CURLOPT_CONNECTTIMEOUT_MS", 0},
#endif
#if HAVE_DECL_CURLOPT_HTTP_TRANSFER_DECODING
    {handleHTTPTransferDecoding, "CURLOPT_HTTP_TRANSFER_DECODING", CURLOPT_HTTP_TRANSFER_DECODING},
#else
    {handleHTTPTransferDecoding, "CURLOPT_HTTP_TRANSFER_DECODING", 0},
#endif
#if HAVE_DECL_CURLOPT_HTTP_CONTENT_DECODING
    {handleHTTPContentDecoding, "CURLOPT_HTTP_CONTENT_DECODING", CURLOPT_HTTP_CONTENT_DECODING},
#else
    {handleHTTPContentDecoding, "CURLOPT_HTTP_CONTENT_DECODING", 0},
#endif
#if HAVE_DECL_CURLOPT_NEW_FILE_PERMS
    {handleNewFilePerms, "CURLOPT_NEW_FILE_PERMS", CURLOPT_NEW_FILE_PERMS},
#else
    {handleNewFilePerms, "CURLOPT_NEW_FILE_PERMS", 0},
#endif
#if HAVE_DECL_CURLOPT_NEW_DIRECTORY_PERMS
    {handleNewDirectoryPerms, "CURLOPT_NEW_DIRECTORY_PERMS", CURLOPT_NEW_DIRECTORY_PERMS},
#else
    {handleNewDirectoryPerms, "CURLOPT_NEW_DIRECTORY_PERMS", 0},
#endif
#if HAVE_DECL_CURLOPT_POST301
    {handlePost301, "CURLOPT_POST301", CURLOPT_POST301},
#else
    {handlePost301, "CURLOPT_POST301", 0},
#endif
#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEY_MD5
    {handleSSHHostPublicKeyMD5, "CURLOPT_SSH_HOST_PUBLIC_KEY_MD5", CURLOPT_SSH_HOST_PUBLIC_KEY_MD5},
#else
    {handleSSHHostPublicKeyMD5, "CURLOPT_SSH_HOST_PUBLIC_KEY_MD5", 0},
#endif
#if HAVE_DECL_CURLOPT_COPYPOSTFIELDS
    {handleCopyPostFields, "CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS},
#else
    {handleCopyPostFields, "CURLOPT_COPYPOSTFIELDS", 0},
#endif
#if HAVE_DECL_CURLOPT_PROXY_TRANSFER_MODE
    {handleProxyTransferMode, "CURLOPT_PROXY_TRANSFER_MODE", CURLOPT_PROXY_TRANSFER_MODE},
#else
    {handleProxyTransferMode, "CURLOPT_PROXY_TRANSFER_MODE", 0},
#endif
#if HAVE_DECL_CURLOPT_SEEKFUNCTION
    {handleSeekFunction, "CURLOPT_SEEKFUNCTION", CURLOPT_SEEKFUNCTION},
#else
    {handleSeekFunction, "CURLOPT_SEEKFUNCTION", 0},
#endif
#if HAVE_DECL_CURLOPT_AUTOREFERER
    {handleAutoReferer, "CURLOPT_AUTOREFERER", CURLOPT_AUTOREFERER},
#else
    {handleAutoReferer, "CURLOPT_AUTOREFERER", 0},
#endif
#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
    {handleOpenSocketFunction, "CURLOPT_OPENSOCKETFUNCTION", CURLOPT_OPENSOCKETFUNCTION},
#else
    {handleOpenSocketFunction, "CURLOPT_OPENSOCKETFUNCTION", 0},
#endif
#if HAVE_DECL_CURLOPT_PROXYTYPE
    {handleProxyType, "CURLOPT_PROXYTYPE", CURLOPT_PROXYTYPE},
#else
    {handleProxyType, "CURLOPT_PROXYTYPE", 0},
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS
    {handleProtocols, "CURLOPT_PROTOCOLS", CURLOPT_PROTOCOLS},
#else
    {handleProtocols, "CURLOPT_PROTOCOLS", 0},
#endif
#if HAVE_DECL_CURLOPT_REDIR_PROTOCOLS
    {handleRedirProtocols, "CURLOPT_REDIR_PROTOCOLS", CURLOPT_REDIR_PROTOCOLS},
#else
    {handleRedirProtocols, "CURLOPT_REDIR_PROTOCOLS", 0},
#endif
#if HAVE_DECL_CURLOPT_RESOLVE
    {handleResolve, "CURLOPT_RESOLVE", CURLOPT_RESOLVE},
#else
    {handleResolve, "CURLOPT_RESOLVE", 0},
#endif
#if HAVE_DECL_CURLOPT_DNS_SERVERS
    {handleDnsServers, "CURLOPT_DNS_SERVERS", CURLOPT_DNS_SERVERS},
#else
    {handleDnsServers, "CURLOPT_DNS_SERVERS", 0},
#endif
};

static char *findOption(CURLOptionMapping optionMap[],
                        CURLoption option)
{
    return optionMap[option].name;
}

static void free_curl_slist(struct curl_slist *slist)
{
    if (NULL == slist)
      return;

    curl_slist_free_all(slist);
}

static void raiseError(Connection *conn, CURLcode code)
{
    CAMLparam0();
    CAMLlocal1(exceptionData);
    value *exception;
    char *errorString = "Unknown Error";
    int i;

    for (i = 0; errorMap[i].name != NULL; i++)
    {
        if (errorMap[i].error == code)
        {
            errorString = errorMap[i].name;
            break;
        }
    }

    exceptionData = caml_alloc(3, 0);

    Store_field(exceptionData, 0, Val_int(code));
    Store_field(exceptionData, 1, Val_int(code));
    Store_field(exceptionData, 2, copy_string(errorString));

    if (conn != NULL && conn->errorBuffer != NULL)
    {
        Store_field(Field(conn->ocamlValues, OcamlErrorBuffer), 0,
		    copy_string(conn->errorBuffer));
    }

    exception = caml_named_value("CurlException");

    if (exception == NULL)
        caml_failwith("CurlException not registered");

    raise_with_arg(*exception, exceptionData);

    CAMLreturn0;
}

static void resetOcamlValues(Connection* connection)
{
    int i;

    for (i = 0; i < OcamlValuesSize; i++)
        Store_field(connection->ocamlValues, i, Val_unit);
}

static Connection* allocConnection(CURL* h)
{
    Connection* connection = (Connection *)malloc(sizeof(Connection));

    connection->ocamlValues = caml_alloc(OcamlValuesSize, 0);
    resetOcamlValues(connection);
    register_global_root(&connection->ocamlValues);

    connection->connection = h;

    connection->next = NULL;
    connection->prev = NULL;

    if (connectionList.tail == NULL)
    {
        connectionList.tail = connection;
        connectionList.head = connection;
    }
    else
    {
        connection->prev = connectionList.head;
        connectionList.head->next = connection;
        connectionList.head = connection;
    }

    connection->refcount = 0;

    connection->url = NULL;
    connection->proxy = NULL;
    connection->userPwd = NULL;
    connection->proxyUserPwd = NULL;
    connection->range = NULL;
    connection->errorBuffer = NULL;
    connection->postFields = NULL;
    connection->postFieldSize = -1;
    connection->referer = NULL;
    connection->userAgent = NULL;
    connection->ftpPort = NULL;
    connection->cookie = NULL;
    connection->httpHeader = NULL;
    connection->httpPostFirst = NULL;
    connection->httpPostLast = NULL;
    connection->httpPostStrings = NULL;
    connection->sslCert = NULL;
    connection->sslCertType = NULL;
    connection->sslCertPasswd = NULL;
    connection->sslKey = NULL;
    connection->sslKeyType = NULL;
    connection->sslKeyPasswd = NULL;
    connection->sslEngine = NULL;
    connection->quote = NULL;
    connection->postQuote = NULL;
    connection->cookieFile = NULL;
    connection->customRequest = NULL;
    connection->interface_ = NULL;
    connection->caInfo = NULL;
    connection->caPath = NULL;
    connection->randomFile = NULL;
    connection->egdSocket = NULL;
    connection->cookieJar = NULL;
    connection->sslCipherList = NULL;
    connection->private = NULL;
    connection->http200Aliases = NULL;
    connection->netrcFile = NULL;
    connection->ftpaccount = NULL;
    connection->cookielist = NULL;
    connection->ftpAlternativeToUser = NULL;
    connection->sshPublicKeyFile = NULL;
    connection->sshPrivateKeyFile = NULL;
    connection->copyPostFields = NULL;
    connection->resolve = NULL;
    connection->dns_servers = NULL;

    return connection;
}

static Connection *newConnection(void)
{
    CURL* h;

    caml_enter_blocking_section();
    h = curl_easy_init();
    caml_leave_blocking_section();

    return allocConnection(h);
}

static Connection *duplicateConnection(Connection *original)
{
    Connection *connection;
    CURL* h;

    caml_enter_blocking_section();
    h  = curl_easy_duphandle(original->connection);
    caml_leave_blocking_section();

    connection = allocConnection(h);

    Store_field(connection->ocamlValues, OcamlWriteCallback,
		Field(original->ocamlValues, OcamlWriteCallback));
    Store_field(connection->ocamlValues, OcamlReadCallback,
		Field(original->ocamlValues, OcamlReadCallback));
    Store_field(connection->ocamlValues, OcamlErrorBuffer,
		Field(original->ocamlValues, OcamlErrorBuffer));
    Store_field(connection->ocamlValues, OcamlPostFields,
		Field(original->ocamlValues, OcamlPostFields));
    Store_field(connection->ocamlValues, OcamlHTTPHeader,
		Field(original->ocamlValues, OcamlHTTPHeader));
    Store_field(connection->ocamlValues, OcamlQuote,
		Field(original->ocamlValues, OcamlQuote));
    Store_field(connection->ocamlValues, OcamlPostQuote,
		Field(original->ocamlValues, OcamlPostQuote));
    Store_field(connection->ocamlValues, OcamlHeaderCallback,
		Field(original->ocamlValues, OcamlHeaderCallback));
    Store_field(connection->ocamlValues, OcamlProgressCallback,
		Field(original->ocamlValues, OcamlProgressCallback));
    Store_field(connection->ocamlValues, OcamlDebugCallback,
		Field(original->ocamlValues, OcamlDebugCallback));
    Store_field(connection->ocamlValues, OcamlHTTP200Aliases,
		Field(original->ocamlValues, OcamlHTTP200Aliases));
    Store_field(connection->ocamlValues, OcamlIOCTLCallback,
		Field(original->ocamlValues, OcamlIOCTLCallback));
    Store_field(connection->ocamlValues, OcamlSeekFunctionCallback,
		Field(original->ocamlValues, OcamlSeekFunctionCallback));

    if (Field(original->ocamlValues, OcamlURL) != Val_unit)
        handleURL(connection, Field(original->ocamlValues,
                                    OcamlURL));
    if (Field(original->ocamlValues, OcamlProxy) != Val_unit)
        handleProxy(connection, Field(original->ocamlValues,
                                      OcamlProxy));
    if (Field(original->ocamlValues, OcamlUserPWD) != Val_unit)
        handleUserPwd(connection, Field(original->ocamlValues,
                                        OcamlUserPWD));
    if (Field(original->ocamlValues, OcamlProxyUserPWD) != Val_unit)
        handleProxyUserPwd(connection, Field(original->ocamlValues,
                                             OcamlProxyUserPWD));
    if (Field(original->ocamlValues, OcamlRange) != Val_unit)
        handleRange(connection, Field(original->ocamlValues,
                                      OcamlRange));
    if (Field(original->ocamlValues, OcamlErrorBuffer) != Val_unit)
        handleErrorBuffer(connection, Field(original->ocamlValues,
                                            OcamlErrorBuffer));
    if (Field(original->ocamlValues, OcamlPostFields) != Val_unit)
        handlePostFields(connection, Field(original->ocamlValues,
                                           OcamlPostFields));
    if (Field(original->ocamlValues, OcamlReferer) != Val_unit)
        handleReferer(connection, Field(original->ocamlValues,
                                        OcamlReferer));
    if (Field(original->ocamlValues, OcamlUserAgent) != Val_unit)
        handleUserAgent(connection, Field(original->ocamlValues,
                                          OcamlUserAgent));
    if (Field(original->ocamlValues, OcamlFTPPort) != Val_unit)
        handleFTPPort(connection, Field(original->ocamlValues,
                                        OcamlFTPPort));
    if (Field(original->ocamlValues, OcamlCookie) != Val_unit)
        handleCookie(connection, Field(original->ocamlValues,
                                       OcamlCookie));
    if (Field(original->ocamlValues, OcamlHTTPHeader) != Val_unit)
        handleHTTPHeader(connection, Field(original->ocamlValues,
                                           OcamlHTTPHeader));
    if (Field(original->ocamlValues, OcamlHTTPPost) != Val_unit)
        handleHTTPPost(connection, Field(original->ocamlValues,
                                         OcamlHTTPPost));
    if (Field(original->ocamlValues, OcamlSSLCert) != Val_unit)
        handleSSLCert(connection, Field(original->ocamlValues,
                                        OcamlSSLCert));
    if (Field(original->ocamlValues, OcamlSSLCertType) != Val_unit)
        handleSSLCertType(connection, Field(original->ocamlValues,
                                            OcamlSSLCertType));
    if (Field(original->ocamlValues, OcamlSSLCertPasswd) != Val_unit)
        handleSSLCertPasswd(connection, Field(original->ocamlValues,
                                              OcamlSSLCertPasswd));
    if (Field(original->ocamlValues, OcamlSSLKey) != Val_unit)
        handleSSLKey(connection, Field(original->ocamlValues,
                                       OcamlSSLKey));
    if (Field(original->ocamlValues, OcamlSSLKeyType) != Val_unit)
        handleSSLKeyType(connection, Field(original->ocamlValues,
                                           OcamlSSLKeyType));
    if (Field(original->ocamlValues, OcamlSSLKeyPasswd) != Val_unit)
        handleSSLKeyPasswd(connection, Field(original->ocamlValues,
                                             OcamlSSLKeyPasswd));
    if (Field(original->ocamlValues, OcamlSSLEngine) != Val_unit)
        handleSSLEngine(connection, Field(original->ocamlValues,
                                          OcamlSSLEngine));
    if (Field(original->ocamlValues, OcamlQuote) != Val_unit)
        handleQuote(connection, Field(original->ocamlValues,
                                      OcamlQuote));
    if (Field(original->ocamlValues, OcamlPostQuote) != Val_unit)
        handlePostQuote(connection, Field(original->ocamlValues,
                                          OcamlPostQuote));
    if (Field(original->ocamlValues, OcamlCookieFile) != Val_unit)
        handleCookieFile(connection, Field(original->ocamlValues,
                                           OcamlCookieFile));
    if (Field(original->ocamlValues, OcamlCustomRequest) != Val_unit)
        handleCustomRequest(connection, Field(original->ocamlValues,
                                              OcamlCustomRequest));
    if (Field(original->ocamlValues, OcamlInterface) != Val_unit)
        handleInterface(connection, Field(original->ocamlValues,
                                          OcamlInterface));
    if (Field(original->ocamlValues, OcamlCAInfo) != Val_unit)
        handleCAInfo(connection, Field(original->ocamlValues,
                                       OcamlCAInfo));
    if (Field(original->ocamlValues, OcamlCAPath) != Val_unit)
        handleCAPath(connection, Field(original->ocamlValues,
                                       OcamlCAPath));
    if (Field(original->ocamlValues, OcamlRandomFile) != Val_unit)
        handleRandomFile(connection, Field(original->ocamlValues,
                                           OcamlRandomFile));
    if (Field(original->ocamlValues, OcamlEGDSocket) != Val_unit)
        handleEGDSocket(connection, Field(original->ocamlValues,
                                          OcamlEGDSocket));
    if (Field(original->ocamlValues, OcamlCookieJar) != Val_unit)
        handleCookieJar(connection, Field(original->ocamlValues,
                                          OcamlCookieJar));
    if (Field(original->ocamlValues, OcamlSSLCipherList) != Val_unit)
        handleSSLCipherList(connection, Field(original->ocamlValues,
                                              OcamlSSLCipherList));
    if (Field(original->ocamlValues, OcamlPrivate) != Val_unit)
        handlePrivate(connection, Field(original->ocamlValues,
                                        OcamlPrivate));
    if (Field(original->ocamlValues, OcamlHTTP200Aliases) != Val_unit)
        handleHTTP200Aliases(connection, Field(original->ocamlValues,
                                               OcamlHTTP200Aliases));
    if (Field(original->ocamlValues, OcamlNETRCFile) != Val_unit)
        handleNETRCFile(connection, Field(original->ocamlValues,
                                          OcamlNETRCFile));
    if (Field(original->ocamlValues, OcamlFTPAccount) != Val_unit)
        handleFTPAccount(connection, Field(original->ocamlValues,
                                           OcamlFTPAccount));
    if (Field(original->ocamlValues, OcamlCookieList) != Val_unit)
        handleCookieList(connection, Field(original->ocamlValues,
                                           OcamlCookieList));
    if (Field(original->ocamlValues, OcamlFTPAlternativeToUser) != Val_unit)
        handleFTPAlternativeToUser(connection,
                                   Field(original->ocamlValues,
                                         OcamlFTPAlternativeToUser));
    if (Field(original->ocamlValues, OcamlSSHPublicKeyFile) != Val_unit)
        handleSSHPublicKeyFile(connection,
                               Field(original->ocamlValues,
                                     OcamlSSHPublicKeyFile));
    if (Field(original->ocamlValues, OcamlSSHPrivateKeyFile) != Val_unit)
        handleSSHPrivateKeyFile(connection,
                                Field(original->ocamlValues,
                                      OcamlSSHPrivateKeyFile));
    if (Field(original->ocamlValues, OcamlCopyPostFields) != Val_unit)
        handleCopyPostFields(connection,
                             Field(original->ocamlValues,
                                   OcamlCopyPostFields));

    return connection;
}

static void free_if(void* p) { if (NULL != p) free(p); }

static void removeConnection(Connection *connection, int finalization)
{
    const char* fin_url = NULL;

    if (!connection->connection)
    {
      return; /* already cleaned up */
    }

    if (finalization)
    {
      /* cannot engage OCaml runtime at finalization, just report leak */
      if (CURLE_OK != curl_easy_getinfo(connection->connection, CURLINFO_EFFECTIVE_URL, &fin_url) || NULL == fin_url)
      {
        fin_url = "unknown";
      }
      fprintf(stderr,"Curl: handle %p leaked, conn %p, url %s\n", connection->connection, connection, fin_url);
      fflush(stderr);
    }
    else
    {
      enter_blocking_section();
      curl_easy_cleanup(connection->connection);
      leave_blocking_section();
    }

    connection->connection = NULL;

    if (connectionList.tail == connection)
        connectionList.tail = connectionList.tail->next;
    if (connectionList.head == connection)
        connectionList.head = connectionList.head->prev;

    if (connection->next != NULL)
        connection->next->prev = connection->prev;
    if (connection->prev != NULL)
        connection->prev->next = connection->next;

    remove_global_root(&connection->ocamlValues);

    free_if(connection->url);
    free_if(connection->proxy);
    free_if(connection->userPwd);
    free_if(connection->proxyUserPwd);
    free_if(connection->range);
    free_if(connection->errorBuffer);
    free_if(connection->postFields);
    free_if(connection->referer);
    free_if(connection->userAgent);
    free_if(connection->ftpPort);
    free_if(connection->cookie);
    free_curl_slist(connection->httpHeader);
    if (connection->httpPostFirst != NULL)
        curl_formfree(connection->httpPostFirst);
    free_curl_slist(connection->httpPostStrings);
    free_curl_slist(connection->resolve);
    free_if(connection->sslCert);
    free_if(connection->sslCertType);
    free_if(connection->sslCertPasswd);
    free_if(connection->sslKey);
    free_if(connection->sslKeyType);
    free_if(connection->sslKeyPasswd);
    free_if(connection->sslEngine);
    free_curl_slist(connection->quote);
    free_curl_slist(connection->postQuote);
    free_if(connection->cookieFile);
    free_if(connection->customRequest);
    free_if(connection->interface_);
    free_if(connection->caInfo);
    free_if(connection->caPath);
    free_if(connection->randomFile);
    free_if(connection->egdSocket);
    free_if(connection->cookieJar);
    free_if(connection->sslCipherList);
    free_if(connection->private);
    free_curl_slist(connection->http200Aliases);
    free_if(connection->netrcFile);
    free_if(connection->ftpaccount);
    free_if(connection->cookielist);
    free_if(connection->ftpAlternativeToUser);
    free_if(connection->sshPublicKeyFile);
    free_if(connection->sshPrivateKeyFile);
    free_if(connection->copyPostFields);
    free_if(connection->dns_servers);
}

#if 1
static void checkConnection(Connection * connection)
{
  (void)connection;
}
#else
static void checkConnection(Connection *connection)
{
    Connection *listIter;

    listIter = connectionList.tail;

    while (listIter != NULL)
    {
        if (listIter == connection)
            return;

        listIter = listIter->next;
    }

    failwith("Invalid Connection");
}
#endif

static Connection* findConnection(CURL* h)
{
    Connection *listIter;

    listIter = connectionList.tail;

    while (listIter != NULL)
    {
        if (listIter->connection == h)
            return listIter;

        listIter = listIter->next;
    }

    failwith("Unknown handle");
}

void op_curl_easy_finalize(value v)
{
  Connection* conn = Connection_val(v);
  /* same connection may be referenced by several different
     OCaml values, see e.g. caml_curl_multi_remove_finished */
  conn->refcount--;
  if (0 == conn->refcount)
  {
    removeConnection(conn, 1);
    free(conn);
  }
}

int op_curl_easy_compare(value v1, value v2)
{
  size_t p1 = (size_t)Connection_val(v1);
  size_t p2 = (size_t)Connection_val(v2);
  return (p1 == p2 ? 0 : (p1 > p2 ? 1 : -1)); /* compare addresses */
}

intnat op_curl_easy_hash(value v)
{
  return (size_t)Connection_val(v); /* address */
}

static struct custom_operations curl_easy_ops = {
  "ygrek.curl_easy",
  op_curl_easy_finalize,
  op_curl_easy_compare,
  op_curl_easy_hash,
  custom_serialize_default,
  custom_deserialize_default,
#if defined(custom_compare_ext_default)
  custom_compare_ext_default,
#endif
};

value caml_curl_alloc(Connection* conn)
{
  value v = caml_alloc_custom(&curl_easy_ops, sizeof(Connection*), 0, 1);
  Connection_val(v) = conn;
  conn->refcount++;
  return v;
}

#define WRAP_DATA_CALLBACK(f) \
static size_t f(char *ptr, size_t size, size_t nmemb, void *data)\
{\
    size_t result;\
    leave_blocking_section();\
    result = f##_nolock(ptr,size,nmemb,data);\
    enter_blocking_section();\
    return result;\
}

static size_t writeFunction_nolock(char *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal2(result, str);
    Connection *conn = (Connection *)data;
    size_t i;

    checkConnection(conn);

    str = alloc_string(size*nmemb);

    for (i = 0; i < size*nmemb; i++)
        Byte(str, i) = ptr[i];

    result = callback_exn(Field(conn->ocamlValues, OcamlWriteCallback), str);

    CAMLreturnT(size_t, Is_exception_result(result) ? 0 : Int_val(result));
}

WRAP_DATA_CALLBACK(writeFunction)

static size_t readFunction_nolock(void *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    size_t length;

    checkConnection(conn);

    result = callback_exn(Field(conn->ocamlValues, OcamlReadCallback),
                      Val_int(size*nmemb));

    if (Is_exception_result(result))
    {
      CAMLreturnT(size_t,CURL_READFUNC_ABORT);
    }

    length = string_length(result);

    if (length <= size*nmemb)
    {
      memcpy(ptr, String_val(result), length);

      CAMLreturnT(size_t,length);
    }
    else
    {
      CAMLreturnT(size_t,CURL_READFUNC_ABORT);
    }
}

WRAP_DATA_CALLBACK(readFunction)

static size_t headerFunction_nolock(char *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal2(result,str);
    Connection *conn = (Connection *)data;
    size_t i;

    checkConnection(conn);

    str = alloc_string(size*nmemb);

    for (i = 0; i < size*nmemb; i++)
        Byte(str, i) = ptr[i];

    result = callback_exn(Field(conn->ocamlValues, OcamlHeaderCallback), str);

    CAMLreturnT(size_t, Is_exception_result(result) ? 0 : Int_val(result));
}

WRAP_DATA_CALLBACK(headerFunction)

static int progressFunction_nolock(void *data,
                            double dlTotal,
                            double dlNow,
                            double ulTotal,
                            double ulNow)
{
    CAMLparam0();
    CAMLlocal1(result);
    CAMLlocalN(callbackData, 4);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    callbackData[0] = copy_double(dlTotal);
    callbackData[1] = copy_double(dlNow);
    callbackData[2] = copy_double(ulTotal);
    callbackData[3] = copy_double(ulNow);

    result = callbackN_exn(Field(conn->ocamlValues, OcamlProgressCallback),
                       4, callbackData);

    CAMLreturnT(int, Is_exception_result(result) ? 1 : Bool_val(result));
}

static int progressFunction(void *data,
                            double dlTotal,
                            double dlNow,
                            double ulTotal,
                            double ulNow)
{
  int r;
  leave_blocking_section();
  r = progressFunction_nolock(data,dlTotal,dlNow,ulTotal,ulNow);
  enter_blocking_section();
  return r;
}

static int debugFunction_nolock(CURL *debugConnection,
                         curl_infotype infoType,
                         char *buffer,
                         size_t bufferLength,
                         void *data)
{
    CAMLparam0();
    CAMLlocal3(camlDebugConnection, camlInfoType, camlMessage);
    size_t i;
    Connection *conn = (Connection *)data;
    (void)debugConnection; /* not used */

    checkConnection(conn);

    camlDebugConnection = (value)conn;
    camlInfoType = Val_long(infoType);
    camlMessage = alloc_string(bufferLength);

    for (i = 0; i < bufferLength; i++)
        Byte(camlMessage, i) = buffer[i];

    callback3_exn(Field(conn->ocamlValues, OcamlDebugCallback),
              camlDebugConnection,
              camlInfoType,
              camlMessage);

    CAMLreturnT(int, 0);
}

static int debugFunction(CURL *debugConnection,
                         curl_infotype infoType,
                         char *buffer,
                         size_t bufferLength,
                         void *data)
{
  int r;
  leave_blocking_section();
  r = debugFunction_nolock(debugConnection, infoType, buffer, bufferLength, data);
  enter_blocking_section();
  return r;
}

static curlioerr ioctlFunction_nolock(CURL *ioctl,
                               int cmd,
                               void *data)
{
    CAMLparam0();
    CAMLlocal3(camlResult, camlConnection, camlCmd);
    Connection *conn = (Connection *)data;
    curlioerr result = CURLIOE_OK;
    (void)ioctl; /* not used */

    checkConnection(conn);

    if (cmd == CURLIOCMD_NOP)
        camlCmd = Val_long(0);
    else if (cmd == CURLIOCMD_RESTARTREAD)
        camlCmd = Val_long(1);
    else
        failwith("Invalid IOCTL Cmd!");

    camlConnection = caml_curl_alloc(conn);

    camlResult = callback2_exn(Field(conn->ocamlValues, OcamlIOCTLCallback),
                           camlConnection,
                           camlCmd);

    if (Is_exception_result(camlResult))
    {
      result = CURLIOE_FAILRESTART;
    }
    else
    switch (Long_val(camlResult))
    {
    case 0: /* CURLIOE_OK */
        result = CURLIOE_OK;
        break;

    case 1: /* CURLIOE_UNKNOWNCMD */
        result = CURLIOE_UNKNOWNCMD;
        break;

    case 2: /* CURLIOE_FAILRESTART */
        result = CURLIOE_FAILRESTART;
        break;

    default: /* Incorrect return value, but let's handle it */
        result = CURLIOE_FAILRESTART;
        break;
    }

    CAMLreturnT(curlioerr, result);
}

static curlioerr ioctlFunction(CURL *ioctl,
                               int cmd,
                               void *data)
{
  curlioerr r;
  leave_blocking_section();
  r = ioctlFunction_nolock(ioctl, cmd, data);
  enter_blocking_section();
  return r;
}

#if HAVE_DECL_CURLOPT_SEEKFUNCTION
static int seekFunction_nolock(void *data,
                        curl_off_t offset,
                        int origin)
{
    CAMLparam0();
    CAMLlocal3(camlResult, camlOffset, camlOrigin);
    Connection *conn = (Connection *)data;

    camlOffset = copy_int64(offset);

    if (origin == SEEK_SET)
        camlOrigin = Val_long(0);
    else if (origin == SEEK_CUR)
        camlOrigin = Val_long(1);
    else if (origin == SEEK_END)
        camlOrigin = Val_long(2);
    else
        failwith("Invalid seek code");

    camlResult = callback2_exn(Field(conn->ocamlValues,
                                 OcamlSeekFunctionCallback),
                           camlOffset,
                           camlOrigin);

    int result;
    if (Is_exception_result(camlResult))
      result = CURL_SEEKFUNC_FAIL;
    else
    switch (Int_val(camlResult))
    {
      case 0: result = CURL_SEEKFUNC_OK; break;
      case 1: result = CURL_SEEKFUNC_FAIL; break;
      case 2: result = CURL_SEEKFUNC_CANTSEEK; break;
      default: failwith("Invalid seek result");
    }

    CAMLreturnT(int, result);
}

static int seekFunction(void *data,
                        curl_off_t offset,
                        int origin)
{
  int r;
  leave_blocking_section();
  r = seekFunction_nolock(data,offset,origin);
  enter_blocking_section();
  return r;
}

#endif

#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
static int openSocketFunction_nolock(void *data,
                        curlsocktype purpose,
                        struct curl_sockaddr *addr)
{
    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    int sock = -1;
    (void)purpose; /* not used */

    sock = socket(addr->family, addr->socktype, addr->protocol);

    if (-1 != sock)
    {
      /* FIXME windows */
      result = callback_exn(Field(conn->ocamlValues, OcamlOpenSocketFunctionCallback), Val_int(sock));
      if (Is_exception_result(result))
      {
        close(sock);
        sock = -1;
      }
    }

    CAMLreturnT(int, (sock == -1) ? CURL_SOCKET_BAD : sock);
}

static int openSocketFunction(void *data,
                        curlsocktype purpose,
                        struct curl_sockaddr *address)
{
  int r;
  leave_blocking_section();
  r = openSocketFunction_nolock(data,purpose,address);
  enter_blocking_section();
  return r;
}

#endif

/**
 **  curl_global_init helper function
 **/

CAMLprim value helper_curl_global_init(value initOption)
{
    CAMLparam1(initOption);

    switch (Long_val(initOption))
    {
    case 0: /* CURLINIT_GLOBALALL */
        CAMLreturn(Val_long(curl_global_init(CURL_GLOBAL_ALL)));
        break;

    case 1: /* CURLINIT_GLOBALSSL */
        CAMLreturn(Val_long(curl_global_init(CURL_GLOBAL_SSL)));
        break;

    case 2: /* CURLINIT_GLOBALWIN32 */
        CAMLreturn(Val_long(curl_global_init(CURL_GLOBAL_WIN32)));
        break;

    case 3: /* CURLINIT_GLOBALNOTHING */
        CAMLreturn(Val_long(curl_global_init(CURL_GLOBAL_NOTHING)));
        break;

    default:
        failwith("Invalid Initialization Option");
        break;
    }

    /* Keep compiler happy, we should never get here due to failwith() */
    CAMLreturn(Val_unit);
}

/**
 **  curl_global_cleanup helper function
 **/

CAMLprim value helper_curl_global_cleanup(void)
{
    CAMLparam0();

    curl_global_cleanup();

    CAMLreturn(Val_unit);
}

/**
 ** curl_easy_init helper function
 **/
CAMLprim value helper_curl_easy_init(void)
{
    CAMLparam0();
    CAMLlocal1(result);

    result = caml_curl_alloc(newConnection());

    CAMLreturn(result);
}

CAMLprim value helper_curl_easy_reset(value conn)
{
    CAMLparam1(conn);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);
    curl_easy_reset(connection->connection);
    resetOcamlValues(connection);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_setopt helper utility functions
 **/

static void handleWriteFunction(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlWriteCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_WRITEFUNCTION,
                              writeFunction);

    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_WRITEDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleReadFunction(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlReadCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_READFUNCTION,
                              readFunction);

    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_READDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleURL(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlURL, option);

    if (conn->url != NULL)
        free(conn->url);

    conn->url = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_URL,
                              conn->url);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleInFileSize(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_INFILESIZE,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleProxy(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlProxy, option);

    if (conn->proxy != NULL)
        free(conn->proxy);

    conn->proxy = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXY,
                              conn->proxy);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleProxyPort(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXYPORT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHTTPProxyTunnel(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTPPROXYTUNNEL,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleVerbose(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_VERBOSE,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHeader(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HEADER,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleNoProgress(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NOPROGRESS,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleNoSignal(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_NOSIGNAL
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NOSIGNAL,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_NOSIGNAL")
    failwith("libcurl does not implement CURLOPT_NOSIGNAL");
#endif
}

static void handleNoBody(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NOBODY,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFailOnError(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FAILONERROR,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleUpload(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_UPLOAD,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePost(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POST,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFTPListOnly(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTPLISTONLY,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFTPAppend(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTPAPPEND,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleNETRC(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    long netrc;

    switch (Long_val(option))
    {
    case 0: /* CURL_NETRC_OPTIONAL */
        netrc = CURL_NETRC_OPTIONAL;
        break;

    case 1:/* CURL_NETRC_IGNORED */
        netrc = CURL_NETRC_IGNORED;
        break;

    case 2: /* CURL_NETRC_REQUIRED */
        netrc = CURL_NETRC_REQUIRED;
        break;

    default:
        failwith("Invalid NETRC Option");
        break;
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NETRC,
                              netrc);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleEncoding(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_ENCODING
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURL_ENCODING_NONE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_ENCODING,
                                  "identity");
        break;

    case 1: /* CURL_ENCODING_DEFLATE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_ENCODING,
                                  "deflate");
        break;

    case 2: /* CURL_ENCODING_GZIP */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_ENCODING,
                                  "gzip");
        break;

    case 3: /* CURL_ENCODING_ANY */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_ENCODING,
                                  "");
        break;

    default:
        failwith("Invalid Encoding Option");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_ENCODING")
    failwith("libcurl does not implement CURLOPT_ENCODING");
#endif
}

static void handleFollowLocation(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FOLLOWLOCATION,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleTransferText(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_TRANSFERTEXT,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePut(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PUT,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleUserPwd(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlUserPWD, option);

    if (conn->userPwd != NULL)
        free(conn->userPwd);

    conn->userPwd = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_USERPWD,
                              conn->userPwd);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleProxyUserPwd(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlProxyUserPWD, option);

    if (conn->proxyUserPwd != NULL)
        free(conn->proxyUserPwd);

    conn->proxyUserPwd = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXYUSERPWD,
                              conn->proxyUserPwd);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleRange(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlRange, option);

    if (conn->range != NULL)
        free(conn->range);

    conn->range = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_RANGE,
                              conn->range);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleErrorBuffer(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlErrorBuffer, option);

    if (conn->errorBuffer != NULL)
        free(conn->errorBuffer);

    conn->errorBuffer = malloc(sizeof(char) * CURL_ERROR_SIZE);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_ERRORBUFFER,
                              conn->errorBuffer);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleTimeout(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_TIMEOUT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePostFields(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlPostFields, option);

    if (conn->postFields != NULL)
        free(conn->postFields);

    conn->postFields = malloc(string_length(option)+1);
    memcpy(conn->postFields, String_val(option), string_length(option)+1);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POSTFIELDS,
                              conn->postFields);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePostFieldSize(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POSTFIELDSIZE,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleReferer(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlReferer, option);

    if (conn->referer != NULL)
        free(conn->referer);

    conn->referer = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_REFERER,
                              conn->referer);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleUserAgent(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlUserAgent, option);

    if (conn->userAgent != NULL)
        free(conn->userAgent);

    conn->userAgent = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_USERAGENT,
                              conn->userAgent);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFTPPort(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlFTPPort, option);

    if (conn->ftpPort != NULL)
        free(conn->ftpPort);

    conn->ftpPort = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTPPORT,
                              conn->ftpPort);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleLowSpeedLimit(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_LOW_SPEED_LIMIT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleLowSpeedTime(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_LOW_SPEED_TIME,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleResumeFrom(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_RESUME_FROM,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCookie(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCookie, option);

    if (conn->cookie != NULL)
        free(conn->cookie);

    conn->cookie = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_COOKIE,
                              conn->cookie);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHTTPHeader(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlHTTPHeader, option);

    free_curl_slist(conn->httpHeader);
    conn->httpHeader = NULL;

    listIter = option;

    while (!Is_long(listIter))
    {
        conn->httpHeader = curl_slist_append(conn->httpHeader, String_val(Field(listIter, 0)));

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTPHEADER,
                              conn->httpHeader);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHTTPPost(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal3(listIter, formItem, contentType);
    CURLcode result = CURLE_OK;
    char *str1, *str2, *str3, *str4;

    listIter = option;

    Store_field(conn->ocamlValues, OcamlHTTPPost, option);

    if (conn->httpPostFirst != NULL)
        curl_formfree(conn->httpPostFirst);

    conn->httpPostFirst = NULL;
    conn->httpPostLast = NULL;

    free_curl_slist(conn->httpPostStrings);
    conn->httpPostStrings = NULL;

    while (!Is_long(listIter))
    {
        formItem = Field(listIter, 0);

        switch (Tag_val(formItem))
        {
        case 0: /* CURLFORM_CONTENT */
            if (Wosize_val(formItem) < 3)
            {
                failwith("Incorrect CURLFORM_CONTENT parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_PTRCONTENTS,
                             str2,
                             CURLFORM_CONTENTSLENGTH,
                             string_length(Field(formItem, 1)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                contentType = Field(formItem, 2);

                str3 = (char *)malloc(string_length(Field(contentType, 0))+1);
                memcpy(str3,
                       String_val(Field(contentType, 0)),
                       string_length(Field(contentType, 0)));
                str3[string_length(Field(contentType, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str3);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_PTRCONTENTS,
                             str2,
                             CURLFORM_CONTENTSLENGTH,
                             string_length(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             str3,
                             CURLFORM_END);
            }
            else
            {
                failwith("Incorrect CURLFORM_CONTENT parameters");
            }
            break;

        case 1: /* CURLFORM_FILECONTENT */
            if (Wosize_val(formItem) < 3)
            {
                failwith("Incorrect CURLFORM_FILECONTENT parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILECONTENT,
                             str2,
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                contentType = Field(formItem, 2);

                str3 = (char *)malloc(string_length(Field(contentType, 0))+1);
                memcpy(str3,
                       String_val(Field(contentType, 0)),
                       string_length(Field(contentType, 0)));
                str3[string_length(Field(contentType, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str3);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILECONTENT,
                             str2,
                             CURLFORM_CONTENTTYPE,
                             str3,
                             CURLFORM_END);
            }
            else
            {
                failwith("Incorrect CURLFORM_FILECONTENT parameters");
            }
            break;

        case 2: /* CURLFORM_FILE */
            if (Wosize_val(formItem) < 3)
            {
                failwith("Incorrect CURLFORM_FILE parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILE,
                             str2,
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                contentType = Field(formItem, 2);

                str3 = (char *)malloc(string_length(Field(contentType, 0))+1);
                memcpy(str3,
                       String_val(Field(contentType, 0)),
                       string_length(Field(contentType, 0)));
                str3[string_length(Field(contentType, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str3);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILE,
                             str2,
                             CURLFORM_CONTENTTYPE,
                             str3,
                             CURLFORM_END);
            }
            else
            {
                failwith("Incorrect CURLFORM_FILE parameters");
            }
            break;

        case 3: /* CURLFORM_BUFFER */
            if (Wosize_val(formItem) < 4)
            {
                failwith("Incorrect CURLFORM_BUFFER parameters");
            }

            if (Is_long(Field(formItem, 3)) &&
                Long_val(Field(formItem, 3)) == 0)
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                str3 = (char *)malloc(string_length(Field(formItem, 2))+1);
                memcpy(str3,
                       String_val(Field(formItem, 2)),
                       string_length(Field(formItem, 2)));
                str3[string_length(Field(formItem, 2))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str3);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             str2,
                             CURLFORM_BUFFERPTR,
                             str3,
                             CURLFORM_BUFFERLENGTH,
                             string_length(Field(formItem, 2)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 3)))
            {
                str1 = (char *)malloc(string_length(Field(formItem, 0))+1);
                memcpy(str1,
                       String_val(Field(formItem, 0)),
                       string_length(Field(formItem, 0)));
                str1[string_length(Field(formItem, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str1);

                str2 = (char *)malloc(string_length(Field(formItem, 1))+1);
                memcpy(str2,
                       String_val(Field(formItem, 1)),
                       string_length(Field(formItem, 1)));
                str2[string_length(Field(formItem, 1))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str2);

                str3 = (char *)malloc(string_length(Field(formItem, 2))+1);
                memcpy(str3,
                       String_val(Field(formItem, 2)),
                       string_length(Field(formItem, 2)));
                str3[string_length(Field(formItem, 2))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str3);

                contentType = Field(formItem, 3);

                str4 = (char *)malloc(string_length(Field(contentType, 0))+1);
                memcpy(str4,
                       String_val(Field(contentType, 0)),
                       string_length(Field(contentType, 0)));
                str4[string_length(Field(contentType, 0))] = 0;
                conn->httpPostStrings =
                    curl_slist_append(conn->httpPostStrings, str4);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_PTRNAME,
                             str1,
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             str2,
                             CURLFORM_BUFFERPTR,
                             str3,
                             CURLFORM_BUFFERLENGTH,
                             string_length(Field(formItem, 2)),
                             CURLFORM_CONTENTTYPE,
                             str4,
                             CURLFORM_END);
            }
            else
            {
                failwith("Incorrect CURLFORM_BUFFER parameters");
            }
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTPPOST,
                              conn->httpPostFirst);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLCert(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLCert, option);

    if (conn->sslCert != NULL)
        free(conn->sslCert);

    conn->sslCert = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLCERT,
                              conn->sslCert);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLCertType(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLCertType, option);

    if (conn->sslCertType != NULL)
        free(conn->sslCertType);

    conn->sslCertType = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLCERTTYPE,
                              conn->sslCertType);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLCertPasswd(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLCertPasswd, option);

    if (conn->sslCertPasswd != NULL)
        free(conn->sslCertPasswd);

    conn->sslCertPasswd = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLCERTPASSWD,
                              conn->sslCertPasswd);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLKey(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLKey, option);

    if (conn->sslKey != NULL)
        free(conn->sslKey);

    conn->sslKey = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLKEY,
                              conn->sslKey);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLKeyType(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLKeyType, option);

    if (conn->sslKeyType != NULL)
        free(conn->sslKeyType);

    conn->sslKeyType = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLKEYTYPE,
                              conn->sslKeyType);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLKeyPasswd(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLKeyPasswd, option);

    if (conn->sslKeyPasswd != NULL)
        free(conn->sslKeyPasswd);

    conn->sslKeyPasswd = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLKEYPASSWD,
                              conn->sslKeyPasswd);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLEngine(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLEngine, option);

    if (conn->sslEngine != NULL)
        free(conn->sslEngine);

    conn->sslEngine = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLENGINE,
                              conn->sslEngine);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLEngineDefault(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLENGINE_DEFAULT,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCRLF(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CRLF,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleQuote(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlQuote, option);

    free_curl_slist(conn->quote);
    conn->quote = NULL;

    listIter = option;

    while (!Is_long(listIter))
    {
        conn->quote = curl_slist_append(conn->quote, String_val(Field(listIter, 0)));

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_QUOTE,
                              conn->quote);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePostQuote(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlPostQuote, option);

    free_curl_slist(conn->postQuote);
    conn->postQuote = NULL;

    listIter = option;

    while (!Is_long(listIter))
    {
        conn->postQuote = curl_slist_append(conn->postQuote, String_val(Field(listIter, 0)));

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POSTQUOTE,
                              conn->postQuote);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHeaderFunction(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlHeaderCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HEADERFUNCTION,
                              headerFunction);

    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_WRITEHEADER,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCookieFile(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCookieFile, option);

    if (conn->cookieFile != NULL)
        free(conn->cookieFile);

    conn->cookieFile = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_COOKIEFILE,
                              conn->cookieFile);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLVersion(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSLVERSION,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleTimeCondition(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    int timecond = CURL_TIMECOND_NONE;

    switch (Long_val(option))
    {
    case 0: timecond = CURL_TIMECOND_NONE; break;
    case 1: timecond = CURL_TIMECOND_IFMODSINCE; break;
    case 2: timecond = CURL_TIMECOND_IFUNMODSINCE; break;
    case 3: timecond = CURL_TIMECOND_LASTMOD; break;
    default:
        failwith("Invalid TIMECOND Option");
        break;
    }

    result = curl_easy_setopt(conn->connection, CURLOPT_TIMECONDITION, timecond);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleTimeValue(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_TIMEVALUE,
                              Int32_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCustomRequest(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCustomRequest, option);

    if (conn->customRequest != NULL)
        free(conn->customRequest);

    conn->customRequest = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CUSTOMREQUEST,
                              conn->customRequest);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleInterface(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlInterface, option);

    if (conn->interface_ != NULL)
        free(conn->interface_);

    conn->interface_ = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_INTERFACE,
                              conn->interface_);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleKRB4Level(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* KRB4_NONE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_KRB4LEVEL,
                                  NULL);
        break;

    case 1: /* KRB4_CLEAR */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_KRB4LEVEL,
                                  "clear");
        break;

    case 2: /* KRB4_SAFE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_KRB4LEVEL,
                                  "safe");
        break;

    case 3: /* KRB4_CONFIDENTIAL */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_KRB4LEVEL,
                                  "confidential");
        break;

    case 4: /* KRB4_PRIVATE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_KRB4LEVEL,
                                  "private");
        break;

    default:
        failwith("Invalid KRB4 Option");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleProgressFunction(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlProgressCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROGRESSFUNCTION,
                              progressFunction);
    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROGRESSDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLVerifyPeer(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSL_VERIFYPEER,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCAInfo(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCAInfo, option);

    if (conn->caInfo != NULL)
        free(conn->caInfo);

    conn->caInfo = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CAINFO,
                              conn->caInfo);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCAPath(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCAPath, option);

    if (conn->caPath != NULL)
        free(conn->caPath);

    conn->caPath = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CAPATH,
                              conn->caPath);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFileTime(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FILETIME,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleMaxRedirs(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAXREDIRS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleMaxConnects(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAXCONNECTS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleClosePolicy(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CLOSEPOLICY_OLDEST */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_CLOSEPOLICY,
                                  CURLCLOSEPOLICY_OLDEST);
        break;

    case 1: /* CLOSEPOLICY_LEAST_RECENTLY_USED */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_CLOSEPOLICY,
                                  CURLCLOSEPOLICY_LEAST_RECENTLY_USED);
        break;

    default:
        failwith("Invalid CLOSEPOLICY Option");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFreshConnect(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FRESH_CONNECT,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleForbidReuse(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FORBID_REUSE,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleRandomFile(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlRandomFile, option);

    if (conn->randomFile != NULL)
        free(conn->randomFile);

    conn->randomFile = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_RANDOM_FILE,
                              conn->randomFile);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleEGDSocket(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlEGDSocket, option);

    if (conn->egdSocket != NULL)
        free(conn->egdSocket);

    conn->egdSocket = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_EGDSOCKET,
                              conn->egdSocket);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleConnectTimeout(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CONNECTTIMEOUT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHTTPGet(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTPGET,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLVerifyHost(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* SSLVERIFYHOST_NONE */
    case 1: /* SSLVERIFYHOST_EXISTENCE */
    case 2: /* SSLVERIFYHOST_HOSTNAME */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_SSL_VERIFYHOST,
                                  /* map EXISTENCE to HOSTNAME */
                                  Long_val(option) == 0 ? 0 : 2);
        break;

    default:
        failwith("Invalid SSLVERIFYHOST Option");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleCookieJar(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCookieJar, option);

    if (conn->cookieJar != NULL)
        free(conn->cookieJar);

    conn->cookieJar = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_COOKIEJAR,
                              conn->cookieJar);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleSSLCipherList(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSLCipherList, option);

    if (conn->sslCipherList != NULL)
        free(conn->sslCipherList);

    conn->sslCipherList = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSL_CIPHER_LIST,
                              conn->sslCipherList);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleHTTPVersion(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* HTTP_VERSION_NONE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_HTTP_VERSION,
                                  CURL_HTTP_VERSION_NONE);
        break;

    case 1: /* HTTP_VERSION_1_0 */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_HTTP_VERSION,
                                  CURL_HTTP_VERSION_1_0);
        break;

    case 2: /* HTTP_VERSION_1_1 */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_HTTP_VERSION,
                                  CURL_HTTP_VERSION_1_1);
        break;

    default:
        failwith("Invalid HTTP_VERSION Option");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleFTPUseEPSV(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_USE_EPSV,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleDNSCacheTimeout(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_DNS_CACHE_TIMEOUT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleDNSUseGlobalCache(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_DNS_USE_GLOBAL_CACHE,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleDebugFunction(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlDebugCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_DEBUGFUNCTION,
                              debugFunction);
    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_DEBUGDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handlePrivate(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_PRIVATE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlPrivate, option);

    if (conn->private != NULL)
        free(conn->private);

    conn->private = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PRIVATE,
                              conn->private);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_PRIVATE")
    failwith("libcurl does not implement CURLOPT_PRIVATE");
#endif
}

static void handleHTTP200Aliases(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_HTTP200ALIASES
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlHTTP200Aliases, option);

    free_curl_slist(conn->http200Aliases);
    conn->http200Aliases = NULL;

    listIter = option;

    while (!Is_long(listIter))
    {
        conn->http200Aliases = curl_slist_append(conn->http200Aliases, String_val(Field(listIter, 0)));

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTP200ALIASES,
                              conn->http200Aliases);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_HTTP200ALIASES")
    failwith("libcurl does not implement CURLOPT_HTTP200ALIASES");
#endif
}

static void handleUnrestrictedAuth(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_UNRESTRICTED_AUTH
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_UNRESTRICTED_AUTH,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_UNRESTRICTED_AUTH")
    failwith("libcurl does not implement CURLOPT_UNRESTRICTED_AUTH");
#endif
}

static void handleFTPUseEPRT(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_USE_EPRT
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_USE_EPRT,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_USE_EPRT")
    failwith("libcurl does not implement CURLOPT_FTP_USE_EPRT");
#endif
}

static void handleHTTPAuth(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_HTTPAUTH
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;
    long auth = CURLAUTH_NONE;

    listIter = option;

    while (!Is_long(listIter))
    {
        switch (Long_val(Field(listIter, 0)))
        {
        case 0: /* CURLAUTH_BASIC */
            auth |= CURLAUTH_BASIC;
            break;

        case 1: /* CURLAUTH_DIGEST */
            auth |= CURLAUTH_DIGEST;
            break;

        case 2: /* CURLAUTH_GSSNEGOTIATE */
            auth |= CURLAUTH_GSSNEGOTIATE;
            break;

        case 3: /* CURLAUTH_NTLM */
            auth |= CURLAUTH_NTLM;
            break;

        case 4: /* CURLAUTH_ANY */
            auth |= CURLAUTH_ANY;
            break;

        case 5: /* CURLAUTH_ANYSAFE */
            auth |= CURLAUTH_ANYSAFE;
            break;

        default:
            failwith("Invalid HTTPAUTH Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTPAUTH,
                              auth);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_HTTPAUTH")
    failwith("libcurl does not implement CURLOPT_HTTPAUTH");
#endif
}

static void handleFTPCreateMissingDirs(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_CREATE_MISSING_DIRS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_CREATE_MISSING_DIRS,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_CREATE_MISSING_DIRS")
    failwith("libcurl does not implement CURLOPT_FTP_CREATE_MISSING_DIRS");
#endif
}

static void handleProxyAuth(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_PROXYAUTH
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;
    long auth = CURLAUTH_NONE;

    listIter = option;

    while (!Is_long(listIter))
    {
        switch (Long_val(Field(listIter, 0)))
        {
        case 0: /* CURLAUTH_BASIC */
            auth |= CURLAUTH_BASIC;
            break;

        case 1: /* CURLAUTH_DIGEST */
            auth |= CURLAUTH_DIGEST;
            break;

        case 2: /* CURLAUTH_GSSNEGOTIATE */
            auth |= CURLAUTH_GSSNEGOTIATE;
            break;

        case 3: /* CURLAUTH_NTLM */
            auth |= CURLAUTH_NTLM;
            break;

        case 4: /* CURLAUTH_ANY */
            auth |= CURLAUTH_ANY;
            break;

        case 5: /* CURLAUTH_ANYSAFE */
            auth |= CURLAUTH_ANYSAFE;
            break;

        default:
            failwith("Invalid HTTPAUTH Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXYAUTH,
                              auth);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_PROXYAUTH")
    failwith("libcurl does not implement CURLOPT_PROXYAUTH");
#endif
}

static void handleFTPResponseTimeout(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_RESPONSE_TIMEOUT
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_RESPONSE_TIMEOUT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_RESPONSE_TIMEOUT")
    failwith("libcurl does not implement CURLOPT_FTP_RESPONSE_TIMEOUT");
#endif
}

static void handleIPResolve(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_IPRESOLVE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURL_IPRESOLVE_WHATEVER */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_WHATEVER);
        break;

    case 1: /* CURL_IPRESOLVE_V4 */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_V4);
        break;

    case 2: /* CURL_IPRESOLVE_V6 */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_V6);
        break;

    default:
        failwith("Invalid IPRESOLVE Value");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_IPRESOLVE")
    failwith("libcurl does not implement CURLOPT_IPRESOLVE");
#endif
}

static void handleMaxFileSize(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_MAXFILESIZE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAXFILESIZE,
                              Int32_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_MAXFILESIZE")
    failwith("libcurl does not implement CURLOPT_MAXFILESIZE");
#endif
}

static void handleInFileSizeLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_INFILESIZE_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_INFILESIZE_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_INFILESIZE_LARGE")
    failwith("libcurl does not implement CURLOPT_INFILESIZE_LARGE");
#endif
}

static void handleResumeFromLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_RESUME_FROM_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_RESUME_FROM_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_RESUME_FROM_LARGE")
    failwith("libcurl does not implement CURLOPT_RESUME_FROM_LARGE");
#endif
}

static void handleMaxFileSizeLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_MAXFILESIZE_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAXFILESIZE_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_MAXFILESIZE_LARGE")
    failwith("libcurl does not implement CURLOPT_MAXFILESIZE_LARGE");
#endif
}

static void handleNETRCFile(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_NETRC_FILE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlNETRCFile, option);

    if (conn->netrcFile != NULL)
        free(conn->netrcFile);

    conn->netrcFile = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NETRC_FILE,
                              conn->netrcFile);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_NETRC_FILE")
    failwith("libcurl does not implement CURLOPT_NETRC_FILE");
#endif
}

static void handleFTPSSL(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_SSL
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURLFTPSSL_NONE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_NONE);
        break;

    case 1: /* CURLFTPSSL_TRY */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_TRY);
        break;

    case 2: /* CURLFTPSSL_CONTROL */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_CONTROL);
        break;

    case 3: /* CURLFTPSSL_ALL */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_ALL);
        break;

    default:
        failwith("Invalid FTP_SSL Value");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_SSL")
    failwith("libcurl does not implement CURLOPT_FTP_SSL");
#endif
}

static void handlePostFieldSizeLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_POSTFIELDSIZE_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POSTFIELDSIZE_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_POSTFIELDSIZE_LARGE")
    failwith("libcurl does not implement CURLOPT_POSTFIELDSIZE_LARGE");
#endif
}

static void handleTCPNoDelay(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_TCP_NODELAY
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_TCP_NODELAY,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_TCP_NODELAY")
    failwith("libcurl does not implement CURLOPT_TCP_NODELAY");
#endif
}

static void handleFTPSSLAuth(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTPSSLAUTH
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURLFTPAUTH_DEFAULT */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_DEFAULT);
        break;

    case 1: /* CURLFTPAUTH_SSL */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_SSL);
        break;

    case 2: /* CURLFTPAUTH_TLS */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_TLS);
        break;

    default:
        failwith("Invalid FTPSSLAUTH value");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTPSSLAUTH")
    failwith("libcurl does not implement CURLOPT_FTPSSLAUTH");
#endif
}

static void handleIOCTLFunction(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlIOCTLCallback, option);
    else
        failwith("Not a proper closure");
    
    result = curl_easy_setopt(conn->connection,
                              CURLOPT_IOCTLFUNCTION,
                              ioctlFunction);
    if (result != CURLE_OK)
        raiseError(conn, result);
    
    result = curl_easy_setopt(conn->connection,
                              CURLOPT_DEBUGDATA,
                              conn);
    
    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_IOCTLFUNCTION")
    failwith("libcurl does not implement CURLOPT_IOCTLFUNCTION");
#endif
}

static void handleFTPAccount(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_ACCOUNT
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlFTPAccount, option);

    if (conn->ftpaccount != NULL)
        free(conn->ftpaccount);
    
    conn->ftpaccount = strdup(String_val(option));
    
    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_ACCOUNT,
                              conn->ftpaccount);
    
    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_ACCOUNT")
    failwith("libcurl does not implement CURLOPT_FTP_ACCOUNT");
#endif
}

static void handleCookieList(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_COOKIELIST
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCookieList, option);

    if (conn->cookielist != NULL)
        free(conn->cookielist);
    
    conn->cookielist = strdup(String_val(option));
    
    result = curl_easy_setopt(conn->connection,
                              CURLOPT_COOKIELIST,
                              conn->cookielist);
    
    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_COOKIELIST")
    failwith("libcurl does not implement CURLOPT_COOKIELIST");
#endif
}

static void handleIgnoreContentLength(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_IGNORE_CONTENT_LENGTH
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_IGNORE_CONTENT_LENGTH,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_IGNORE_CONTENT_LENGTH")
    failwith("libcurl does not implement CURLOPT_IGNORE_CONTENT_LENGTH");
#endif
}

static void handleFTPSkipPASVIP(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_SKIP_PASV_IP
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_SKIP_PASV_IP,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_SKIP_PASV_IP")
    failwith("libcurl does not implement CURLOPT_FTP_SKIP_PASV_IP");
#endif
}

static void handleFTPFileMethod(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_FILEMETHOD
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURLFTPMETHOD_DEFAULT */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_DEFAULT);
        break;

    case 1: /* CURLFTMETHOD_MULTICWD */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_MULTICWD);
        break;

    case 2: /* CURLFTPMETHOD_NOCWD */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_NOCWD);
        break;

    case 3: /* CURLFTPMETHOD_SINGLECWD */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_SINGLECWD);

    default:
        failwith("Invalid FTP_FILEMETHOD value");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_FILEMETHOD")
    failwith("libcurl does not implement CURLOPT_FTP_FILEMETHOD");
#endif
}

static void handleLocalPort(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_LOCALPORT
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_LOCALPORT,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_LOCALPORT")
    failwith("libcurl does not implement CURLOPT_LOCALPORT");
#endif
}

static void handleLocalPortRange(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_LOCALPORTRANGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_LOCALPORTRANGE,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_LOCALPORTRANGE")
    failwith("libcurl does not implement CURLOPT_LOCALPORTRANGE");
#endif
}

static void handleConnectOnly(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_CONNECT_ONLY
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CONNECT_ONLY,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_CONNECT_ONLY")
    failwith("libcurl does not implement CURLOPT_CONNECT_ONLY");
#endif
}

static void handleMaxSendSpeedLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_MAX_SEND_SPEED_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAX_SEND_SPEED_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_MAX_SEND_SPEED_LARGE")
    failwith("libcurl does not implement CURLOPT_MAX_SEND_SPEED_LARGE");
#endif
}

static void handleMaxRecvSpeedLarge(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_MAX_RECV_SPEED_LARGE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_MAX_RECV_SPEED_LARGE,
                              Int64_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_MAX_RECV_SPEED_LARGE")
    failwith("libcurl does not implement CURLOPT_MAX_RECV_SPEED_LARGE");
#endif
}

static void handleFTPAlternativeToUser(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_ALTERNATIVE_TO_USER
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlFTPAlternativeToUser, option);

    if (conn->ftpAlternativeToUser != NULL)
        free(conn->ftpAlternativeToUser);

    conn->ftpAlternativeToUser = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_FTP_ALTERNATIVE_TO_USER,
                              conn->ftpAlternativeToUser);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_ALTERNATIVE_TO_USER")
    failwith("libcurl does not implement CURLOPT_FTP_ALTERNATIVE_TO_USER");
#endif
}

static void handleSSLSessionIdCache(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SSL_SESSIONID_CACHE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSL_SESSIONID_CACHE,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SSL_SESSIONID_CACHE")
    failwith("libcurl does not implement CURLOPT_SSL_SESSIONID_CACHE");
#endif
}

static void handleSSHAuthTypes(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SSH_AUTH_TYPES
    CAMLparam1(option);
    CAMLlocal1(listIter);
    CURLcode result = CURLE_OK;
    long authTypes = CURLSSH_AUTH_NONE;

    listIter = option;
    
    while (!Is_long(listIter))
    {
        switch (Long_val(Field(listIter, 0)))
        {
        case 0: /* CURLSSH_AUTH_ANY */
            authTypes |= CURLSSH_AUTH_ANY;
            break;

        case 1: /* CURLSSH_AUTH_PUBLICKEY */
            authTypes |= CURLSSH_AUTH_PUBLICKEY;
            break;

        case 2: /* CURLSSH_AUTH_PASSWORD */
            authTypes |= CURLSSH_AUTH_PASSWORD;
            break;

        case 3: /* CURLSSH_AUTH_HOST */
            authTypes |= CURLSSH_AUTH_HOST;
            break;

        case 4: /* CURLSSH_AUTH_KEYBOARD */
            authTypes |= CURLSSH_AUTH_KEYBOARD;
            break;

        default:
            failwith("Invalid CURLSSH_AUTH_TYPES Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSH_AUTH_TYPES,
                              authTypes);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SSH_AUTH_TYPES")
    failwith("libcurl does not implement CURLOPT_SSH_AUTH_TYPES");
#endif
}

static void handleSSHPublicKeyFile(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEYFILE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSHPublicKeyFile, option);

    if (conn->sshPublicKeyFile != NULL)
        free(conn->sshPublicKeyFile);

    conn->sshPublicKeyFile = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSH_PUBLIC_KEYFILE,
                              conn->sshPublicKeyFile);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SSH_PUBLIC_KEYFILE")
    failwith("libcurl does not implement CURLOPT_SSH_PUBLIC_KEYFILE");
#endif
}

static void handleSSHPrivateKeyFile(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SSH_PRIVATE_KEYFILE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSHPrivateKeyFile, option);

    if (conn->sshPrivateKeyFile != NULL)
        free(conn->sshPrivateKeyFile);

    conn->sshPrivateKeyFile = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSH_PRIVATE_KEYFILE,
                              conn->sshPrivateKeyFile);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SSH_PRIVATE_KEYFILE")
    failwith("libcurl does not implement CURLOPT_SSH_PRIVATE_KEYFILE");
#endif
}

static void handleFTPSSLCCC(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_FTP_SSL_CCC
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURLFTPSSL_CCC_NONE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_NONE);
        break;

    case 1: /* CURLFTPSSL_CCC_PASSIVE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_PASSIVE);
        break;

    case 2: /* CURLFTPSSL_CCC_ACTIVE */
        result = curl_easy_setopt(conn->connection,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_ACTIVE);
        break;

    default:
        failwith("Invalid FTPSSL_CCC value");
        break;
    }

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_FTP_SSL_CCC")
    failwith("libcurl does not implement CURLOPT_FTP_SSL_CCC");
#endif
}

static void handleTimeoutMS(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_TIMEOUT_MS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_TIMEOUT_MS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_TIMEOUT_MS")
    failwith("libcurl does not implement CURLOPT_TIMEOUT_MS");
#endif
}

static void handleConnectTimeoutMS(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_CONNECTTIMEOUT_MS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_CONNECTTIMEOUT_MS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_CONNECTTIMEOUT_MS")
    failwith("libcurl does not implement CURLOPT_CONNECTTIMEOUT_MS");
#endif
}

static void handleHTTPTransferDecoding(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_HTTP_TRANSFER_DECODING
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTP_TRANSFER_DECODING,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_HTTP_TRANSFER_DECODING")
    failwith("libcurl does not implement CURLOPT_HTTP_TRANSFER_DECODING");
#endif
}

static void handleHTTPContentDecoding(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_HTTP_CONTENT_DECODING
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_HTTP_CONTENT_DECODING,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_HTTP_CONTENT_DECODING")
    failwith("libcurl does not implement CURLOPT_HTTP_CONTENT_DECODING");
#endif
}

static void handleNewFilePerms(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_NEW_FILE_PERMS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NEW_FILE_PERMS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_NEW_FILE_PERMS")
    failwith("libcurl does not implement CURLOPT_NEW_FILE_PERMS");
#endif
}

static void handleNewDirectoryPerms(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_NEW_DIRECTORY_PERMS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_NEW_DIRECTORY_PERMS,
                              Long_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_NEW_DIRECTORY_PERMS")
    failwith("libcurl does not implement CURLOPT_NEW_DIRECTORY_PERMS");
#endif
}

static void handlePost301(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_POST301
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POST301,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_POST301")
    failwith("libcurl does not implement CURLOPT_POST301");
#endif
}

static void handleSSHHostPublicKeyMD5(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SSH_HOST_PUBLIC_KEY_MD5
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlSSHHostPublicKeyMD5, option);

    if (conn->sshHostPublicKeyMD5 != NULL)
        free(conn->sshHostPublicKeyMD5);

    conn->sshHostPublicKeyMD5 = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SSH_HOST_PUBLIC_KEY_MD5,
                              conn->sshHostPublicKeyMD5);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SSH_HOST_PUBLIC_KEY_MD5")
    failwith("libcurl does not implement CURLOPT_SSH_HOST_PUBLIC_KEY_MD5");
#endif
}

static void handleCopyPostFields(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_COPYPOSTFIELDS
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlCopyPostFields, option);

    if (conn->copyPostFields != NULL)
        free(conn->copyPostFields);

    conn->copyPostFields = strdup(String_val(option));

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_COPYPOSTFIELDS,
                              conn->copyPostFields);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_COPYPOSTFIELDS")
    failwith("libcurl does not implement CURLOPT_COPYPOSTFIELDS");
#endif
}

static void handleProxyTransferMode(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_PROXY_TRANSFER_MODE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXY_TRANSFER_MODE,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_PROXY_TRANSFER_MODE")
    failwith("libcurl does not implement CURLOPT_PROXY_TRANSFER_MODE");
#endif
}

static void handleSeekFunction(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_SEEKFUNCTION
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    if (Tag_val(option) == Closure_tag)
        Store_field(conn->ocamlValues, OcamlSeekFunctionCallback, option);
    else
        failwith("Not a proper closure");

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SEEKFUNCTION,
                              seekFunction);

    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_SEEKDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_SEEKFUNCTION")
    failwith("libcurl does not implement CURLOPT_SEEKFUNCTION");
#endif
}

static void handleAutoReferer(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_AUTOREFERER
    CAMLparam1(option);
    CURLcode result = curl_easy_setopt(conn->connection,
                              CURLOPT_AUTOREFERER,
                              Bool_val(option));

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_AUTOREFERER")
    failwith("libcurl does not implement CURLOPT_AUTOREFERER");
#endif
}

static void handleOpenSocketFunction(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, OcamlOpenSocketFunctionCallback, option);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_OPENSOCKETDATA,
                              conn);

    if (result != CURLE_OK)
        raiseError(conn, result);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_OPENSOCKETFUNCTION,
                              openSocketFunction);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_OPENSOCKETFUNCTION")
    failwith("libcurl does not implement CURLOPT_OPENSOCKETFUNCTION");
#endif
}

static void handleProxyType(Connection *conn, value option)
{
#if HAVE_DECL_CURLOPT_PROXYTYPE
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    long proxy_type;

    switch (Long_val(option))
    {
      case 0: proxy_type = CURLPROXY_HTTP; break;
      case 1: proxy_type = CURLPROXY_HTTP_1_0; break;
      case 2: proxy_type = CURLPROXY_SOCKS4; break;
      case 3: proxy_type = CURLPROXY_SOCKS5; break;
      case 4: proxy_type = CURLPROXY_SOCKS4A; break;
      case 5: proxy_type = CURLPROXY_SOCKS5_HOSTNAME; break;
      default:
        failwith("Invalid curl proxy type");
    }

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_PROXYTYPE,
                              proxy_type);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
#else
#pragma message("libcurl does not implement CURLOPT_PROXYTYPE")
    failwith("libcurl does not implement CURLOPT_PROXYTYPE");
#endif
}

#if HAVE_DECL_CURLOPT_PROTOCOLS && HAVE_DECL_CURLOPT_REDIR_PROTOCOLS

long protoMap[] =
{
  CURLPROTO_ALL,
  CURLPROTO_HTTP, CURLPROTO_HTTPS, CURLPROTO_FTP, CURLPROTO_FTPS, CURLPROTO_SCP, CURLPROTO_SFTP,
  CURLPROTO_TELNET, CURLPROTO_LDAP, CURLPROTO_LDAPS, CURLPROTO_DICT, CURLPROTO_FILE, CURLPROTO_TFTP,
/* factor out with autoconf? */
#if defined(CURLPROTO_IMAP)
  CURLPROTO_IMAP,
#else
  0,
#endif
#if defined(CURLPROTO_IMAPS)
  CURLPROTO_IMAPS,
#else
  0,
#endif
#if defined(CURLPROTO_POP3)
  CURLPROTO_POP3,
#else
  0,
#endif
#if defined(CURLPROTO_POP3S)
  CURLPROTO_POP3S,
#else
  0,
#endif
#if defined(CURLPROTO_SMTP)
  CURLPROTO_SMTP,
#else
  0,
#endif
#if defined(CURLPROTO_SMTPS)
  CURLPROTO_SMTPS,
#else
  0,
#endif
#if defined(CURLPROTO_RTSP)
  CURLPROTO_RTSP,
#else
  0,
#endif
#if defined(CURLPROTO_RTMP)
  CURLPROTO_RTMP,
#else
  0,
#endif
#if defined(CURLPROTO_RTMPT)
  CURLPROTO_RTMPT,
#else
  0,
#endif
#if defined(CURLPROTO_RTMPE)
  CURLPROTO_RTMPE,
#else
  0,
#endif
#if defined(CURLPROTO_RTMPTE)
  CURLPROTO_RTMPTE,
#else
  0,
#endif
#if defined(CURLPROTO_RTMPS)
  CURLPROTO_RTMPS,
#else
  0,
#endif
#if defined(CURLPROTO_RTMPTS)
  CURLPROTO_RTMPTS,
#else
  0,
#endif
#if defined(CURLPROTO_GOPHER)
  CURLPROTO_GOPHER,
#else
  0,
#endif
};

static void handleProtocolsOption(CURLoption curlopt, Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    long protocols = 0;
    int index;

    while (Val_emptylist != option)
    {
        index = Int_val(Field(option, 0));
        if ((index < 0) || ((size_t)index >= sizeof(protoMap) / sizeof(protoMap[0])))
          failwith("Invalid curl protocol");

        protocols = protocols | protoMap[index];

        option = Field(option, 1);
    }

    result = curl_easy_setopt(conn->connection,
                              curlopt,
                              protocols);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static void handleProtocols(Connection *conn, value option)
{
  handleProtocolsOption(CURLOPT_PROTOCOLS, conn, option);
}

static void handleRedirProtocols(Connection *conn, value option)
{
  handleProtocolsOption(CURLOPT_REDIR_PROTOCOLS, conn, option);
}

#else
#pragma message("libcurl does not implement CURLOPT_PROTOCOLS or CURLOPT_REDIR_PROTOCOLS")
static void handleProtocols(Connection *conn, value option)
{
    failwith("libcurl does not implement CURLOPT_PROTOCOLS");
}
static void handleRedirProtocols(Connection *conn, value option)
{
    failwith("libcurl does not implement CURLOPT_REDIR_PROTOCOLS");
}
#endif

#if HAVE_DECL_CURLOPT_RESOLVE
static void handleResolve(Connection *conn, value option)
{
  CAMLparam1(option);
  CAMLlocal1(head);

  CURLcode result = CURLE_OK;

  free_curl_slist(conn->resolve);
  conn->resolve = NULL;

  head = option;

  while (head != Val_emptylist)
  {
     conn->resolve = curl_slist_append(conn->resolve, String_val(Field(head,0)));
     head = Field(head, 1);
  }

  result = curl_easy_setopt(conn->connection,
                            CURLOPT_RESOLVE,
                            conn->resolve);

  if (result != CURLE_OK)
    raiseError(conn, result);

  CAMLreturn0;
}

#else
#pragma message("libcurl does not implement CURLOPT_RESOLVE")
static void handleResolve(Connection *conn, value option)
{
  failwith("libcurl does not implement CURLOPT_RESOLVE");
}
#endif

#if HAVE_DECL_CURLOPT_DNS_SERVERS
static void handleDnsServers(Connection *conn, value option)
{
  CAMLparam1(option);

  CURLcode result = CURLE_OK;
  free_if(conn->dns_servers);

  conn->dns_servers = strdup(String_val(option));

  result = curl_easy_setopt(conn->connection,
                            CURLOPT_DNS_SERVERS,
                            conn->dns_servers);

  if (result != CURLE_OK)
    raiseError(conn, result);

  CAMLreturn0;
}
#else
#pragma message("libcurl does not implement CURLOPT_DNS_SERVERS")
static void handleDnsServers(Connection *conn, value option)
{
  failwith("libcurl does not implement CURLOPT_DNS_SERVERS");
}
#endif

/**
 **  curl_easy_setopt helper function
 **/

CAMLprim value helper_curl_easy_setopt(value conn, value option)
{
    CAMLparam2(conn, option);
    CAMLlocal1(data);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    if (Is_long(option))
    {
        char error[128];

        sprintf(error, "Unimplemented Option: %s",
                findOption(unimplementedOptionMap,
                           (CURLoption)(Long_val(option))));

        failwith(error);
    }

    if (!Is_block(option))
        failwith("Not a block");

    if (Wosize_val(option) < 1)
        failwith("Insufficient data in block");

    data = Field(option, 0);

    if (Tag_val(option) < sizeof(implementedOptionMap)/sizeof(CURLOptionMapping))
        (*implementedOptionMap[Tag_val(option)].optionHandler)(connection,
                                                               data);
    else
        failwith("Invalid CURLOPT Option");

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_perform helper function
 **/

CAMLprim value helper_curl_easy_perform(value conn)
{
    CAMLparam1(conn);
    CURLcode result = CURLE_OK;
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    enter_blocking_section();
    result = curl_easy_perform(connection->connection);
    leave_blocking_section();

    if (result != CURLE_OK)
        raiseError(connection, result);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_cleanup helper function
 **/

CAMLprim value helper_curl_easy_cleanup(value conn)
{
    CAMLparam1(conn);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    removeConnection(connection, 0);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_duphandle helper function
 **/

CAMLprim value helper_curl_easy_duphandle(value conn)
{
    CAMLparam1(conn);
    CAMLlocal1(result);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    result = caml_curl_alloc(duplicateConnection(connection));

    CAMLreturn(result);
}

/**
 **  curl_easy_getinfo helper function
 **/

enum GetInfoResultType {
    StringValue, LongValue, DoubleValue, StringListValue
};

value convertStringList(struct curl_slist *slist)
{
    CAMLparam0();
    CAMLlocal3(result, current, next);
    struct curl_slist *p = slist;

    result = Val_int(0);
    current = Val_int(0);
    next = Val_int(0);

    while (p != NULL)
    {
        next = alloc_tuple(2);
        Store_field(next, 0, copy_string(p->data));
        Store_field(next, 1, Val_int(0));

        if (result == Val_int(0))
            result = next;

        if (current != Val_int(0))
	    Store_field(current, 1, next);

        current = next;

        p = p->next;
    }

    curl_slist_free_all(slist);

    CAMLreturn(result);
}

CAMLprim value helper_curl_easy_getinfo(value conn, value option)
{
    CAMLparam2(conn, option);
    CAMLlocal1(result);
    CURLcode curlResult;
    Connection *connection = Connection_val(conn);
    enum GetInfoResultType resultType;
    char *strValue = NULL;
    double doubleValue;
    long longValue;
    struct curl_slist *stringListValue = NULL;

    checkConnection(connection);

    switch(Long_val(option))
    {
#if HAVE_DECL_CURLINFO_EFFECTIVE_URL
    case 0: /* CURLINFO_EFFECTIVE_URL */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_EFFECTIVE_URL,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_EFFECTIVE_URL")
#endif

#if HAVE_DECL_CURLINFO_RESPONSE_CODE || HAVE_DECL_CURLINFO_HTTP_CODE
    case 1: /* CURLINFO_HTTP_CODE */
    case 2: /* CURLINFO_RESPONSE_CODE */
#if HAVE_DECL_CURLINFO_RESPONSE_CODE
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_RESPONSE_CODE,
                                       &longValue);
#else
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_HTTP_CODE,
                                       &longValue);
#endif
        break;
#endif

#if HAVE_DECL_CURLINFO_TOTAL_TIME
    case 3: /* CURLINFO_TOTAL_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_TOTAL_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_NAMELOOKUP_TIME
    case 4: /* CURLINFO_NAMELOOKUP_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_NAMELOOKUP_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONNECT_TIME
    case 5: /* CURLINFO_CONNECT_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_CONNECT_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_PRETRANSFER_TIME
    case 6: /* CURLINFO_PRETRANSFER_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_PRETRANSFER_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SIZE_UPLOAD
    case 7: /* CURLINFO_SIZE_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SIZE_UPLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SIZE_DOWNLOAD
    case 8: /* CURLINFO_SIZE_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SIZE_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SPEED_DOWNLOAD
    case 9: /* CURLINFO_SPEED_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SPEED_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SPEED_UPLOAD
    case 10: /* CURLINFO_SPEED_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SPEED_UPLOAD,
                                       &doubleValue);
        break;

#endif

#if HAVE_DECL_CURLINFO_HEADER_SIZE
    case 11: /* CURLINFO_HEADER_SIZE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_HEADER_SIZE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REQUEST_SIZE
    case 12: /* CURLINFO_REQUEST_SIZE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_REQUEST_SIZE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SSL_VERIFYRESULT
    case 13: /* CURLINFO_SSL_VERIFYRESULT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SSL_VERIFYRESULT,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_FILETIME
    case 14: /* CURLINFO_FILETIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_FILETIME,
                                       &longValue);

        doubleValue = longValue;
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_LENGTH_DOWNLOAD
    case 15: /* CURLINFO_CONTENT_LENGTH_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_LENGTH_UPLOAD
    case 16: /* CURLINFO_CONTENT_LENGTH_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_CONTENT_LENGTH_UPLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_STARTTRANSFER_TIME
    case 17: /* CURLINFO_STARTTRANSFER_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_STARTTRANSFER_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_TYPE
    case 18: /* CURLINFO_CONTENT_TYPE */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_CONTENT_TYPE,
                                       &strValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_TIME
    case 19: /* CURLINFO_REDIRECT_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_REDIRECT_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_COUNT
    case 20: /* CURLINFO_REDIRECT_COUNT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_REDIRECT_COUNT,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_PRIVATE
    case 21: /* CURLINFO_PRIVATE */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_PRIVATE,
                                       &strValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_HTTP_CONNECTCODE
    case 22: /* CURLINFO_HTTP_CONNECTCODE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_HTTP_CONNECTCODE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_HTTPAUTH_AVAIL
    case 23: /* CURLINFO_HTTPAUTH_AVAIL */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_HTTPAUTH_AVAIL,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_PROXYAUTH_AVAIL
    case 24: /* CURLINFO_PROXYAUTH_AVAIL */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_PROXYAUTH_AVAIL,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_OS_ERRNO
    case 25: /* CURLINFO_OS_ERRNO */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_OS_ERRNO,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_NUM_CONNECTS
    case 26: /* CURLINFO_NUM_CONNECTS */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_NUM_CONNECTS,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SSL_ENGINES
    case 27: /* CURLINFO_SSL_ENGINES */
        resultType = StringListValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_SSL_ENGINES,
                                       &stringListValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_COOKIELIST
    case 28: /* CURLINFO_COOKIELIST */
        resultType = StringListValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_COOKIELIST,
                                       &stringListValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_LASTSOCKET
    case 29: /* CURLINFO_LASTSOCKET */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_LASTSOCKET,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_FTP_ENTRY_PATH
    case 30: /* CURLINFO_FTP_ENTRY_PATH */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_FTP_ENTRY_PATH,
                                       &strValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_URL
    case 31: /* CURLINFO_REDIRECT_URL */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_REDIRECT_URL,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_REDIRECT_URL")
#endif

#if HAVE_DECL_CURLINFO_PRIMARY_IP
    case 32: /* CURLINFO_PRIMARY_IP */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_PRIMARY_IP,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_PRIMARY_IP")
#endif

#if HAVE_DECL_CURLINFO_LOCAL_IP
    case 33: /* CURLINFO_LOCAL_IP */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_LOCAL_IP,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_LOCAL_IP")
#endif

#if HAVE_DECL_CURLINFO_LOCAL_PORT
    case 34: /* CURLINFO_LOCAL_PORT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_LOCAL_PORT,
                                       &longValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_LOCAL_PORT")
#endif

#if HAVE_DECL_CURLINFO_CONDITION_UNMET
    case 35: /* CURLINFO_CONDITION_UNMET */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->connection,
                                       CURLINFO_CONDITION_UNMET,
                                       &longValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_CONDITION_UNMET")
#endif

    default:
        failwith("Invalid CURLINFO Option");
        break;
    }

    if (curlResult != CURLE_OK)
        raiseError(connection, curlResult);

    switch (resultType)
    {
    case StringValue:
        result = alloc(1, StringValue);
        Store_field(result, 0, copy_string(strValue?strValue:""));
        break;

    case LongValue:
        result = alloc(1, LongValue);
        Store_field(result, 0, Val_long(longValue));
        break;

    case DoubleValue:
        result = alloc(1, DoubleValue);
        Store_field(result, 0, copy_double(doubleValue));
        break;

    case StringListValue:
        result = alloc(1, StringListValue);
        Store_field(result, 0, convertStringList(stringListValue));
        break;
    }

    CAMLreturn(result);
}

/**
 **  curl_escape helper function
 **/

CAMLprim value helper_curl_escape(value str)
{
    CAMLparam1(str);
    CAMLlocal1(result);
    char *curlResult;
     
    curlResult = curl_escape(String_val(str), string_length(str));
    result = copy_string(curlResult);
    free(curlResult);

    CAMLreturn(result);
}

/**
 **  curl_unescape helper function
 **/

CAMLprim value helper_curl_unescape(value str)
{
    CAMLparam1(str);
    CAMLlocal1(result);
    char *curlResult;
     
    curlResult = curl_unescape(String_val(str), string_length(str));
    result = copy_string(curlResult);
    free(curlResult);

    CAMLreturn(result);
}

/**
 **  curl_getdate helper function
 **/

CAMLprim value helper_curl_getdate(value str, value now)
{
    CAMLparam2(str, now);
    CAMLlocal1(result);
    time_t curlResult;
    time_t curlNow;

    curlNow = (time_t)Double_val(now);
    curlResult = curl_getdate(String_val(str), &curlNow);
    result = copy_double((double)curlResult);

    CAMLreturn(result);
}

/**
 **  curl_version helper function
 **/

CAMLprim value helper_curl_version(void)
{
    CAMLparam0();
    CAMLlocal1(result);
    char *str;

    str = curl_version();
    result = copy_string(str);

    CAMLreturn(result);
}

struct CURLVersionBitsMapping
{
    int code;
    char *name;
};

struct CURLVersionBitsMapping versionBitsMap[] =
{
    {CURL_VERSION_IPV6, "ipv6"},
    {CURL_VERSION_KERBEROS4, "kerberos4"},
    {CURL_VERSION_SSL, "ssl"},
    {CURL_VERSION_LIBZ, "libz"},
    {CURL_VERSION_NTLM, "ntlm"},
    {CURL_VERSION_GSSNEGOTIATE, "gssnegotiate"},
    {CURL_VERSION_DEBUG, "debug"},
    {CURL_VERSION_CURLDEBUG, "curldebug"},
    {CURL_VERSION_ASYNCHDNS, "asynchdns"},
    {CURL_VERSION_SPNEGO, "spnego"},
    {CURL_VERSION_LARGEFILE, "largefile"},
    {CURL_VERSION_IDN, "idn"},
    {CURL_VERSION_SSPI, "sspi"},
    {CURL_VERSION_CONV, "conv"},
#if HAVE_DECL_CURL_VERSION_TLSAUTH_SRP
    {CURL_VERSION_TLSAUTH_SRP, "srp"},
#endif
#if HAVE_DECL_CURL_VERSION_NTLM_WB
    {CURL_VERSION_NTLM_WB, "wb"},
#endif
};

CAMLprim value caml_curl_version_info(value unit)
{
  CAMLparam1(unit);
  CAMLlocal4(v, vlist, vnum, vfeatures);
  const char* const* p = NULL;
  size_t i = 0;

  curl_version_info_data* data = curl_version_info(CURLVERSION_NOW);
  if (NULL == data) caml_failwith("curl_version_info");

  vlist = Val_emptylist;
  for (p = data->protocols; NULL != *p; p++)
  {
    vlist = Val_cons(vlist, caml_copy_string(*p));
  }

  vfeatures = Val_emptylist;
  for (i = 0; i < sizeof(versionBitsMap)/sizeof(versionBitsMap[0]); i++)
  {
    if (0 != (versionBitsMap[i].code & data->features))
      vfeatures = Val_cons(vfeatures, caml_copy_string(versionBitsMap[i].name));
  }

  vnum = caml_alloc_tuple(3);
  Store_field(vnum,0,Val_int(0xFF & (data->version_num >> 16)));
  Store_field(vnum,1,Val_int(0xFF & (data->version_num >> 8)));
  Store_field(vnum,2,Val_int(0xFF & (data->version_num)));

  v = caml_alloc_tuple(12);
  Store_field(v,0,caml_copy_string(data->version));
  Store_field(v,1,vnum);
  Store_field(v,2,caml_copy_string(data->host));
  Store_field(v,3,vfeatures);
  Store_field(v,4,data->ssl_version ? Val_some(caml_copy_string(data->ssl_version)) : Val_none);
  Store_field(v,5,data->libz_version ? Val_some(caml_copy_string(data->libz_version)) : Val_none);
  Store_field(v,6,vlist);
  Store_field(v,7,caml_copy_string((data->age >= 1 && data->ares) ? data->ares : ""));
  Store_field(v,8,Val_int((data->age >= 1) ? data->ares_num : 0));
  Store_field(v,9,caml_copy_string((data->age >= 2 && data->libidn) ? data->libidn : ""));
  Store_field(v,10,Val_int((data->age >= 3) ? data->iconv_ver_num : 0));
  Store_field(v,11,caml_copy_string((data->age >= 3 && data->libssh_version) ? data->libssh_version : ""));

  CAMLreturn(v);
}

CAMLprim value caml_curl_pause(value conn, value opts)
{
  CAMLparam2(conn, opts);
  CAMLlocal4(v, vlist, vnum, vfeatures);
  Connection *connection = Connection_val(conn);
  int bitmask = 0;
  CURLcode result;

  while (Val_emptylist != opts)
  {
    switch (Int_val(Field(opts,0)))
    {
      case 0: bitmask |= CURLPAUSE_SEND; break;
      case 1: bitmask |= CURLPAUSE_RECV; break;
      case 2: bitmask |= CURLPAUSE_ALL; break;
      default: caml_failwith("wrong pauseOption");
    }
    opts = Field(opts,1);
  }

  result = curl_easy_pause(connection->connection,bitmask);
  if (result != CURLE_OK)
    raiseError(connection, result);

  CAMLreturn(Val_unit);
}

/*
 * Curl multi stack support
 *
 * Exported thin wrappers for libcurl are prefixed with caml_curl_multi_.
 * Other exported functions are prefixed with caml_curlm_, some of them
 * can/should be decomposed into smaller parts.
 */

struct ml_multi_handle
{
  CURLM* handle;
  value values; /* callbacks */
};

enum
{
  curlmopt_socket_function,
  curlmopt_timer_function,

  /* last, not used */
  multi_values_total
};

typedef struct ml_multi_handle ml_multi_handle;

#define Multi_val(v) (*(ml_multi_handle**)Data_custom_val(v))
#define CURLM_val(v) (Multi_val(v)->handle)

static struct custom_operations curl_multi_ops = {
  "ygrek.curl_multi",
  custom_finalize_default,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default,
#if defined(custom_compare_ext_default)
  custom_compare_ext_default,
#endif
};

CAMLprim value caml_curl_multi_init(value unit)
{
  CAMLparam1(unit);
  CAMLlocal1(v);
  ml_multi_handle* multi = (ml_multi_handle*)caml_stat_alloc(sizeof(ml_multi_handle));
  CURLM* h = curl_multi_init();

  if (!h)
  {
    caml_stat_free(multi);
    failwith("caml_curl_multi_init");
  }

  multi->handle = h;
  multi->values = caml_alloc(multi_values_total, 0);
  caml_register_generational_global_root(&multi->values);

  v = caml_alloc_custom(&curl_multi_ops, sizeof(ml_multi_handle*), 0, 1);
  Multi_val(v) = multi;

  CAMLreturn(v);
}

CAMLprim value caml_curl_multi_cleanup(value handle)
{
  CAMLparam1(handle);
  ml_multi_handle* h = Multi_val(handle);

  if (NULL == h)
    CAMLreturn(Val_unit);

  caml_remove_generational_global_root(&h->values);

  if (CURLM_OK != curl_multi_cleanup(h->handle))
    failwith("caml_curl_multi_cleanup");

  Multi_val(handle) = (ml_multi_handle*)NULL;

  CAMLreturn(Val_unit);
}

static CURL* curlm_remove_finished(CURLM* multi_handle, CURLcode* result)
{
  int msgs_in_queue = 0;

  while (1)
  {
    CURLMsg* msg = curl_multi_info_read(multi_handle, &msgs_in_queue);
    if (NULL == msg) return NULL;
    if (CURLMSG_DONE == msg->msg)
    {
      CURL* easy_handle = msg->easy_handle;
      if (result) *result = msg->data.result;
      if (CURLM_OK != curl_multi_remove_handle(multi_handle, easy_handle))
      {
        /*failwith("curlm_remove_finished");*/
      }
      return easy_handle;
    }
  }
}

CAMLprim value caml_curlm_remove_finished(value v_multi)
{
  CAMLparam1(v_multi);
  CAMLlocal2(v_easy, v_tuple);
  CURL* handle;
  CURLM* multi_handle;
  CURLcode result;
  Connection* conn = NULL;

  multi_handle = CURLM_val(v_multi);

  caml_enter_blocking_section();
  handle = curlm_remove_finished(multi_handle,&result);
  caml_leave_blocking_section();

  if (NULL == handle)
  {
    CAMLreturn(Val_none);
  }
  else
  {
    conn = findConnection(handle);
    if (conn->errorBuffer != NULL)
    {
        Store_field(Field(conn->ocamlValues, OcamlErrorBuffer), 0, caml_copy_string(conn->errorBuffer));
    }
    conn->refcount--;
    /* NB: same handle, but different block */
    v_easy = caml_curl_alloc(conn);
    v_tuple = caml_alloc(2, 0);
    Store_field(v_tuple,0,v_easy);
    Store_field(v_tuple,1,Val_int(result)); /* CURLcode */
    CAMLreturn(Val_some(v_tuple));
  }
}

static int curlm_wait_data(CURLM* multi_handle)
{
	struct timeval timeout;
  CURLMcode ret;

	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	int maxfd = -1;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);

	/* set a suitable timeout */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	/* get file descriptors from the transfers */
	ret = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

	if (ret == CURLM_OK && maxfd >= 0)
	{
		int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
		if (-1 != rc) return 0;
	}
	return 1;
}

CAMLprim value caml_curlm_wait_data(value v_multi)
{
  CAMLparam1(v_multi);
  int ret;
  CURLM* h = CURLM_val(v_multi);

  caml_enter_blocking_section();
  ret = curlm_wait_data(h);
  caml_leave_blocking_section();

  CAMLreturn(Val_bool(0 == ret));
}

CAMLprim value caml_curl_multi_add_handle(value v_multi, value v_easy)
{
  CAMLparam2(v_multi,v_easy);
  CURLM* multi = CURLM_val(v_multi);
  Connection* conn = Connection_val(v_easy);

  /* prevent collection of OCaml value while the easy handle is used
   and may invoke callbacks registered on OCaml side */
  conn->refcount++;

  /* may invoke callbacks so need to be consistent with locks */
  caml_enter_blocking_section();
  if (CURLM_OK != curl_multi_add_handle(multi, conn->connection))
  {
    conn->refcount--; /* not added, revert */
    caml_leave_blocking_section();
    failwith("caml_curl_multi_add_handle");
  }
  caml_leave_blocking_section();

  CAMLreturn(Val_unit);
}

CAMLprim value caml_curl_multi_remove_handle(value v_multi, value v_easy)
{
  CAMLparam2(v_multi,v_easy);
  CURLM* multi = CURLM_val(v_multi);
  Connection* conn = Connection_val(v_easy);

  /* may invoke callbacks so need to be consistent with locks */
  caml_enter_blocking_section();
  if (CURLM_OK != curl_multi_remove_handle(multi, conn->connection))
  {
    caml_leave_blocking_section();
    failwith("caml_curl_multi_remove_handle");
  }
  conn->refcount--;
  caml_leave_blocking_section();

  CAMLreturn(Val_unit);
}

CAMLprim value caml_curl_multi_perform_all(value v_multi)
{
  CAMLparam1(v_multi);
  int still_running = 0;
  CURLM* h = CURLM_val(v_multi);

  caml_enter_blocking_section();
  while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(h, &still_running));
  caml_leave_blocking_section();

  CAMLreturn(Val_int(still_running));
}

CAMLprim value helper_curl_easy_strerror(value v_code)
{
  CAMLparam1(v_code);
  CAMLreturn(caml_copy_string(curl_easy_strerror(Int_val(v_code))));
}

/*
 * Wrappers for the curl_multi_socket_action infrastructure
 * Based on curl hiperfifo.c example
 */

#ifdef _WIN32
#ifndef Val_socket
#define Val_socket(v) win_alloc_socket(v)
#endif
#ifndef Socket_val
#error Socket_val not defined in unixsupport.h
#endif
#else /* _WIN32 */
#ifndef Socket_val
#define Socket_val(v) Long_val(v)
#endif
#ifndef Val_socket
#define Val_socket(v) Val_int(v)
#endif
#endif  /* _WIN32 */

static void raise_error(char const* msg)
{
  static value* exception = NULL;

  if (NULL == exception)
  {
    exception = caml_named_value("Curl.Multi.Error");
    if (NULL == exception) caml_invalid_argument("Curl.Multi.Error");
  }

  caml_raise_with_string(*exception, msg);
}

static void check_mcode(CURLMcode code)
{
  char const *s = NULL;
  switch (code)
  {
    case CURLM_OK                  : return;
    case CURLM_CALL_MULTI_PERFORM  : s="CURLM_CALL_MULTI_PERFORM"; break;
    case CURLM_BAD_HANDLE          : s="CURLM_BAD_HANDLE";         break;
    case CURLM_BAD_EASY_HANDLE     : s="CURLM_BAD_EASY_HANDLE";    break;
    case CURLM_OUT_OF_MEMORY       : s="CURLM_OUT_OF_MEMORY";      break;
    case CURLM_INTERNAL_ERROR      : s="CURLM_INTERNAL_ERROR";     break;
    case CURLM_UNKNOWN_OPTION      : s="CURLM_UNKNOWN_OPTION";     break;
    case CURLM_LAST                : s="CURLM_LAST";               break;
    case CURLM_BAD_SOCKET          : s="CURLM_BAD_SOCKET";         break;
    default                        : s="CURLM_unknown";            break;
  }
  raise_error(s);
}

CAMLprim value caml_curl_multi_socket_action(value v_multi, value v_fd, value v_kind)
{
  CAMLparam3(v_multi, v_fd, v_kind);
  CURLM* h = CURLM_val(v_multi);
  int still_running = 0;
  CURLMcode rc = CURLM_OK;
  curl_socket_t socket;
  int kind = 0;

  if (Val_none == v_fd)
  {
    socket = CURL_SOCKET_TIMEOUT;
  }
  else
  {
    socket = Socket_val(Field(v_fd, 0));
  }

  switch (Int_val(v_kind))
  {
    case 0 : break;
    case 1 : kind |= CURL_CSELECT_IN; break;
    case 2 : kind |= CURL_CSELECT_OUT; break;
    case 3 : kind |= CURL_CSELECT_IN | CURL_CSELECT_OUT; break;
    default:
      raise_error("caml_curl_multi_socket_action");
  }

/*  fprintf(stdout,"fd %u kind %u\n",socket, kind); fflush(stdout); */

  caml_enter_blocking_section();
  do {
    rc = curl_multi_socket_action(h, socket, kind, &still_running);
  } while (rc == CURLM_CALL_MULTI_PERFORM);
  caml_leave_blocking_section();

  check_mcode(rc);

  CAMLreturn(Val_int(still_running));
}

CAMLprim value caml_curl_multi_socket_all(value v_multi)
{
  CAMLparam1(v_multi);
  int still_running = 0;
  CURLMcode rc = CURLM_OK;
  CURLM* h = CURLM_val(v_multi);

  caml_enter_blocking_section();
  do {
    rc = curl_multi_socket_all(h, &still_running);
  } while (rc == CURLM_CALL_MULTI_PERFORM);
  caml_leave_blocking_section();

  check_mcode(rc);

  CAMLreturn(Val_int(still_running));
}

static int curlm_sock_cb_nolock(CURL *e, curl_socket_t sock, int what, ml_multi_handle* multi, void *sockp)
{
  CAMLparam0();
  CAMLlocal2(v_what,csock);
  (void)e;
  (void)sockp; /* not used */

  /* v_what = Val_int(what); */
  switch (what)
  {
    case CURL_POLL_NONE   : v_what = Val_int(0); break;
    case CURL_POLL_IN     : v_what = Val_int(1); break;
    case CURL_POLL_OUT    : v_what = Val_int(2); break;
    case CURL_POLL_INOUT  : v_what = Val_int(3); break;
    case CURL_POLL_REMOVE : v_what = Val_int(4); break;
    default:
      fprintf(stderr, "curlm_sock_cb sock=%d what=%d\n", sock, what);
      fflush(stderr);
      raise_error("curlm_sock_cb"); /* FIXME exception from callback */
  }
  csock=Val_socket(sock);
  caml_callback2(Field(multi->values,curlmopt_socket_function),
                 csock, v_what);

  CAMLreturn(0);
}

static int curlm_sock_cb(CURL *e, curl_socket_t sock, int what, void *cbp, void *sockp)
{
  int ret;
  caml_leave_blocking_section();
  ret = curlm_sock_cb_nolock(e, sock, what, (ml_multi_handle*)cbp, sockp);
  caml_enter_blocking_section();
  return ret;
}

CAMLprim value caml_curl_multi_socketfunction(value v_multi, value v_cb)
{
  CAMLparam2(v_multi, v_cb);
  ml_multi_handle* multi = Multi_val(v_multi);

  Store_field(multi->values, curlmopt_socket_function, v_cb);

  curl_multi_setopt(multi->handle, CURLMOPT_SOCKETFUNCTION, curlm_sock_cb);
  curl_multi_setopt(multi->handle, CURLMOPT_SOCKETDATA, multi);

  CAMLreturn(Val_unit);
}

static void curlm_timer_cb_nolock(ml_multi_handle *multi, long timeout_ms)
{
  CAMLparam0();
  caml_callback(Field(multi->values,curlmopt_timer_function), Val_long(timeout_ms));
  CAMLreturn0;
}

static int curlm_timer_cb(CURLM *multi, long timeout_ms, void *userp)
{
  (void)multi;

  caml_leave_blocking_section();
  curlm_timer_cb_nolock((ml_multi_handle*)userp, timeout_ms);
  caml_enter_blocking_section();
  return 0;
}

CAMLprim value caml_curl_multi_timerfunction(value v_multi, value v_cb)
{
  CAMLparam2(v_multi, v_cb);
  ml_multi_handle* multi = Multi_val(v_multi);

  Store_field(multi->values, curlmopt_timer_function, v_cb);

  curl_multi_setopt(multi->handle, CURLMOPT_TIMERFUNCTION, curlm_timer_cb);
  curl_multi_setopt(multi->handle, CURLMOPT_TIMERDATA, multi);

  CAMLreturn(Val_unit);
}

CAMLprim value caml_curl_multi_timeout(value v_multi)
{
  CAMLparam1(v_multi);
  long ms = 0;
  CURLMcode rc = CURLM_OK;
  ml_multi_handle* multi = Multi_val(v_multi);

  rc = curl_multi_timeout(multi->handle, &ms);

  check_mcode(rc);

  CAMLreturn(Val_long(ms));
}

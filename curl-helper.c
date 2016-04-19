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
/* suppress false gcc warning on seekFunction */
#define CURL_DISABLE_TYPECHECK
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

typedef enum OcamlValues
{
    Ocaml_WRITEFUNCTION,
    Ocaml_READFUNCTION,
    Ocaml_ERRORBUFFER,
    Ocaml_POSTFIELDS,
    Ocaml_HTTPHEADER,
    Ocaml_HTTPPOST,
    Ocaml_QUOTE,
    Ocaml_POSTQUOTE,
    Ocaml_HEADERFUNCTION,
    Ocaml_PROGRESSFUNCTION,
    Ocaml_DEBUGFUNCTION,
    Ocaml_HTTP200ALIASES,
    Ocaml_IOCTLFUNCTION,
    Ocaml_SEEKFUNCTION,
    Ocaml_OPENSOCKETFUNCTION,

    Ocaml_URL,
    Ocaml_PROXY,
    Ocaml_USERPWD,
    Ocaml_PROXYUSERPWD,
    Ocaml_RANGE,
    Ocaml_REFERER,
    Ocaml_USERAGENT,
    Ocaml_FTPPORT,
    Ocaml_COOKIE,
    Ocaml_HTTPPOSTSTRINGS,
    Ocaml_SSLCERT,
    Ocaml_SSLCERTTYPE,
    Ocaml_SSLCERTPASSWD,
    Ocaml_SSLKEY,
    Ocaml_SSLKEYTYPE,
    Ocaml_SSLKEYPASSWD,
    Ocaml_SSLENGINE,
    Ocaml_COOKIEFILE,
    Ocaml_CUSTOMREQUEST,
    Ocaml_INTERFACE,
    Ocaml_CAINFO,
    Ocaml_CAPATH,
    Ocaml_RANDOM_FILE,
    Ocaml_EGDSOCKET,
    Ocaml_COOKIEJAR,
    Ocaml_SSL_CIPHER_LIST,
    Ocaml_PRIVATE,
    Ocaml_NETRC_FILE,
    Ocaml_FTP_ACCOUNT,
    Ocaml_COOKIELIST,
    Ocaml_FTP_ALTERNATIVE_TO_USER,
    Ocaml_SSH_PUBLIC_KEYFILE,
    Ocaml_SSH_PRIVATE_KEYFILE,
    Ocaml_SSH_HOST_PUBLIC_KEY_MD5,
    Ocaml_COPYPOSTFIELDS,

    Ocaml_DNS_SERVERS,

    Ocaml_MAIL_FROM,
    Ocaml_MAIL_RCPT,

    Ocaml_RESOLVE,

    /* Not used, last for size */
    OcamlValuesSize
} OcamlValue;

struct Connection
{
    CURL *connection;
    Connection *next;
    Connection *prev;

    value ocamlValues;

    size_t refcount; /* number of references to this structure */

    char *curl_URL;
    char *curl_PROXY;
    char *curl_USERPWD;
    char *curl_PROXYUSERPWD;
    char *curl_RANGE;
    char *curl_ERRORBUFFER;
    char *curl_POSTFIELDS;
    int curl_POSTFIELDSIZE;
    char *curl_REFERER;
    char *curl_USERAGENT;
    char *curl_FTPPORT;
    char *curl_COOKIE;
    struct curl_slist *curl_HTTPHEADER;
    struct curl_slist *httpPostBuffers;
    struct curl_httppost *httpPostFirst;
    struct curl_httppost *httpPostLast;
    struct curl_slist *curl_RESOLVE;
    char *curl_SSLCERT;
    char *curl_SSLCERTTYPE;
    char *curl_SSLCERTPASSWD;
    char *curl_SSLKEY;
    char *curl_SSLKEYTYPE;
    char *curl_SSLKEYPASSWD;
    char *curl_SSLENGINE;
    struct curl_slist *curl_QUOTE;
    struct curl_slist *curl_POSTQUOTE;
    char *curl_COOKIEFILE;
    char *curl_CUSTOMREQUEST;
    char *curl_INTERFACE;
    char *curl_CAINFO;
    char *curl_CAPATH;
    char *curl_RANDOM_FILE;
    char *curl_EGDSOCKET;
    char *curl_COOKIEJAR;
    char *curl_SSL_CIPHER_LIST;
    char *curl_PRIVATE;
    struct curl_slist *curl_HTTP200ALIASES;
    char *curl_NETRC_FILE;
    char *curl_FTP_ACCOUNT;
    char *curl_COOKIELIST;
    char *curl_FTP_ALTERNATIVE_TO_USER;
    char *curl_SSH_PUBLIC_KEYFILE;
    char *curl_SSH_PRIVATE_KEYFILE;
    char *curl_SSH_HOST_PUBLIC_KEY_MD5;
    char *curl_COPYPOSTFIELDS;
    char *curl_DNS_SERVERS;
    char *curl_MAIL_FROM;
    struct curl_slist *curl_MAIL_RCPT;
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
    OcamlValue ocamlValue;
};

static char* strdup_ml(value v)
{
  char* p = NULL;
  p = malloc(caml_string_length(v)+1);
  memcpy(p,String_val(v),caml_string_length(v)+1); // caml strings have terminating zero
  return p;
}

/* prepends to the beginning of list */
static struct curl_slist* curl_slist_prepend_ml(struct curl_slist* list, value v)
{
  /* FIXME check NULLs */
  struct curl_slist* new_item = malloc(sizeof(struct curl_slist));

  new_item->next = list;
  new_item->data = strdup_ml(v);

  return new_item;
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

    if (conn != NULL && conn->curl_ERRORBUFFER != NULL)
    {
        Store_field(Field(conn->ocamlValues, Ocaml_ERRORBUFFER), 0,
		    copy_string(conn->curl_ERRORBUFFER));
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

    connection->curl_URL = NULL;
    connection->curl_PROXY = NULL;
    connection->curl_USERPWD = NULL;
    connection->curl_PROXYUSERPWD = NULL;
    connection->curl_RANGE = NULL;
    connection->curl_ERRORBUFFER = NULL;
    connection->curl_POSTFIELDS = NULL;
    connection->curl_POSTFIELDSIZE = -1;
    connection->curl_REFERER = NULL;
    connection->curl_USERAGENT = NULL;
    connection->curl_FTPPORT = NULL;
    connection->curl_COOKIE = NULL;
    connection->curl_HTTPHEADER = NULL;
    connection->httpPostBuffers = NULL;
    connection->httpPostFirst = NULL;
    connection->httpPostLast = NULL;
    connection->curl_SSLCERT = NULL;
    connection->curl_SSLCERTTYPE = NULL;
    connection->curl_SSLCERTPASSWD = NULL;
    connection->curl_SSLKEY = NULL;
    connection->curl_SSLKEYTYPE = NULL;
    connection->curl_SSLKEYPASSWD = NULL;
    connection->curl_SSLENGINE = NULL;
    connection->curl_QUOTE = NULL;
    connection->curl_POSTQUOTE = NULL;
    connection->curl_COOKIEFILE = NULL;
    connection->curl_CUSTOMREQUEST = NULL;
    connection->curl_INTERFACE = NULL;
    connection->curl_CAINFO = NULL;
    connection->curl_CAPATH = NULL;
    connection->curl_RANDOM_FILE = NULL;
    connection->curl_EGDSOCKET = NULL;
    connection->curl_COOKIEJAR = NULL;
    connection->curl_SSL_CIPHER_LIST = NULL;
    connection->curl_PRIVATE = NULL;
    connection->curl_HTTP200ALIASES = NULL;
    connection->curl_NETRC_FILE = NULL;
    connection->curl_FTP_ACCOUNT = NULL;
    connection->curl_COOKIELIST = NULL;
    connection->curl_FTP_ALTERNATIVE_TO_USER = NULL;
    connection->curl_SSH_PUBLIC_KEYFILE = NULL;
    connection->curl_SSH_PRIVATE_KEYFILE = NULL;
    connection->curl_COPYPOSTFIELDS = NULL;
    connection->curl_RESOLVE = NULL;
    connection->curl_DNS_SERVERS = NULL;
    connection->curl_MAIL_FROM = NULL;
    connection->curl_MAIL_RCPT = NULL;

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

    free_if(connection->curl_URL);
    free_if(connection->curl_PROXY);
    free_if(connection->curl_USERPWD);
    free_if(connection->curl_PROXYUSERPWD);
    free_if(connection->curl_RANGE);
    free_if(connection->curl_ERRORBUFFER);
    free_if(connection->curl_POSTFIELDS);
    free_if(connection->curl_REFERER);
    free_if(connection->curl_USERAGENT);
    free_if(connection->curl_FTPPORT);
    free_if(connection->curl_COOKIE);
    free_curl_slist(connection->curl_HTTPHEADER);
    free_curl_slist(connection->httpPostBuffers);
    if (connection->httpPostFirst != NULL)
        curl_formfree(connection->httpPostFirst);
    free_curl_slist(connection->curl_RESOLVE);
    free_if(connection->curl_SSLCERT);
    free_if(connection->curl_SSLCERTTYPE);
    free_if(connection->curl_SSLCERTPASSWD);
    free_if(connection->curl_SSLKEY);
    free_if(connection->curl_SSLKEYTYPE);
    free_if(connection->curl_SSLKEYPASSWD);
    free_if(connection->curl_SSLENGINE);
    free_curl_slist(connection->curl_QUOTE);
    free_curl_slist(connection->curl_POSTQUOTE);
    free_if(connection->curl_COOKIEFILE);
    free_if(connection->curl_CUSTOMREQUEST);
    free_if(connection->curl_INTERFACE);
    free_if(connection->curl_CAINFO);
    free_if(connection->curl_CAPATH);
    free_if(connection->curl_RANDOM_FILE);
    free_if(connection->curl_EGDSOCKET);
    free_if(connection->curl_COOKIEJAR);
    free_if(connection->curl_SSL_CIPHER_LIST);
    free_if(connection->curl_PRIVATE);
    free_curl_slist(connection->curl_HTTP200ALIASES);
    free_if(connection->curl_NETRC_FILE);
    free_if(connection->curl_FTP_ACCOUNT);
    free_if(connection->curl_COOKIELIST);
    free_if(connection->curl_FTP_ALTERNATIVE_TO_USER);
    free_if(connection->curl_SSH_PUBLIC_KEYFILE);
    free_if(connection->curl_SSH_PRIVATE_KEYFILE);
    free_if(connection->curl_COPYPOSTFIELDS);
    free_if(connection->curl_DNS_SERVERS);
    free_if(connection->curl_MAIL_FROM);
    free_curl_slist(connection->curl_MAIL_RCPT);
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

#define WRAP_DATA_CALLBACK(name) \
static size_t cb_##name(char *ptr, size_t size, size_t nmemb, void *data)\
{\
    size_t result;\
    leave_blocking_section();\
    result = cb_##name##_nolock(ptr,size,nmemb,data);\
    enter_blocking_section();\
    return result;\
}

static size_t cb_WRITEFUNCTION_nolock(char *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal2(result, str);
    Connection *conn = (Connection *)data;
    size_t i;

    checkConnection(conn);

    str = alloc_string(size*nmemb);

    for (i = 0; i < size*nmemb; i++)
        Byte(str, i) = ptr[i];

    result = callback_exn(Field(conn->ocamlValues, Ocaml_WRITEFUNCTION), str);

    CAMLreturnT(size_t, Is_exception_result(result) ? 0 : Int_val(result));
}

WRAP_DATA_CALLBACK( WRITEFUNCTION)

static size_t cb_READFUNCTION_nolock(void *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    size_t length;

    checkConnection(conn);

    result = callback_exn(Field(conn->ocamlValues, Ocaml_READFUNCTION),
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

WRAP_DATA_CALLBACK( READFUNCTION)

static size_t cb_HEADERFUNCTION_nolock(char *ptr, size_t size, size_t nmemb, void *data)
{
    CAMLparam0();
    CAMLlocal2(result,str);
    Connection *conn = (Connection *)data;
    size_t i;

    checkConnection(conn);

    str = alloc_string(size*nmemb);

    for (i = 0; i < size*nmemb; i++)
        Byte(str, i) = ptr[i];

    result = callback_exn(Field(conn->ocamlValues, Ocaml_HEADERFUNCTION), str);

    CAMLreturnT(size_t, Is_exception_result(result) ? 0 : Int_val(result));
}

WRAP_DATA_CALLBACK( HEADERFUNCTION)

static int cb_PROGRESSFUNCTION_nolock(void *data,
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

    result = callbackN_exn(Field(conn->ocamlValues, Ocaml_PROGRESSFUNCTION),
                       4, callbackData);

    CAMLreturnT(int, Is_exception_result(result) ? 1 : Bool_val(result));
}

static int cb_PROGRESSFUNCTION(void *data,
                            double dlTotal,
                            double dlNow,
                            double ulTotal,
                            double ulNow)
{
  int r;
  leave_blocking_section();
  r = cb_PROGRESSFUNCTION_nolock(data,dlTotal,dlNow,ulTotal,ulNow);
  enter_blocking_section();
  return r;
}

static int cb_DEBUGFUNCTION_nolock(CURL *debugConnection,
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

    callback3_exn(Field(conn->ocamlValues, Ocaml_DEBUGFUNCTION),
              camlDebugConnection,
              camlInfoType,
              camlMessage);

    CAMLreturnT(int, 0);
}

static int cb_DEBUGFUNCTION(CURL *debugConnection,
                         curl_infotype infoType,
                         char *buffer,
                         size_t bufferLength,
                         void *data)
{
  int r;
  leave_blocking_section();
  r = cb_DEBUGFUNCTION_nolock(debugConnection, infoType, buffer, bufferLength, data);
  enter_blocking_section();
  return r;
}

static curlioerr cb_IOCTLFUNCTION_nolock(CURL *ioctl,
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

    camlResult = callback2_exn(Field(conn->ocamlValues, Ocaml_IOCTLFUNCTION),
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

static curlioerr cb_IOCTLFUNCTION(CURL *ioctl,
                               int cmd,
                               void *data)
{
  curlioerr r;
  leave_blocking_section();
  r = cb_IOCTLFUNCTION_nolock(ioctl, cmd, data);
  enter_blocking_section();
  return r;
}

#if HAVE_DECL_CURLOPT_SEEKFUNCTION
static int cb_SEEKFUNCTION_nolock(void *data,
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
                                 Ocaml_SEEKFUNCTION),
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

static int cb_SEEKFUNCTION(void *data,
                        curl_off_t offset,
                        int origin)
{
  int r;
  leave_blocking_section();
  r = cb_SEEKFUNCTION_nolock(data,offset,origin);
  enter_blocking_section();
  return r;
}

#endif

#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
static int cb_OPENSOCKETFUNCTION_nolock(void *data,
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
      result = callback_exn(Field(conn->ocamlValues, Ocaml_OPENSOCKETFUNCTION), Val_int(sock));
      if (Is_exception_result(result))
      {
        close(sock);
        sock = -1;
      }
    }

    CAMLreturnT(int, (sock == -1) ? CURL_SOCKET_BAD : sock);
}

static int cb_OPENSOCKETFUNCTION(void *data,
                        curlsocktype purpose,
                        struct curl_sockaddr *address)
{
  int r;
  leave_blocking_section();
  r = cb_OPENSOCKETFUNCTION_nolock(data,purpose,address);
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

#define SETOPT_FUNCTION(name) \
static void handle_##name##FUNCTION(Connection *conn, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLE_OK; \
    Store_field(conn->ocamlValues, Ocaml_##name##FUNCTION, option); \
    result = curl_easy_setopt(conn->connection, CURLOPT_##name##FUNCTION, cb_##name##FUNCTION); \
    if (result != CURLE_OK) raiseError(conn, result); \
    result = curl_easy_setopt(conn->connection, CURLOPT_##name##DATA, conn); \
    if (result != CURLE_OK) raiseError(conn, result); \
    CAMLreturn0; \
}

SETOPT_FUNCTION( WRITE)
SETOPT_FUNCTION( READ)
SETOPT_FUNCTION( HEADER)
SETOPT_FUNCTION( PROGRESS)
SETOPT_FUNCTION( DEBUG)

#if HAVE_DECL_CURLOPT_SEEKFUNCTION
SETOPT_FUNCTION( SEEK)
#endif

#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
SETOPT_FUNCTION( IOCTL)
#endif

#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
SETOPT_FUNCTION( OPENSOCKET)
#endif

static void handle_slist(Connection *conn, struct curl_slist** slist, OcamlValue caml_option, CURLoption curl_option, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, caml_option, option);

    free_curl_slist(*slist);
    *slist = NULL;

    while (Val_emptylist != option)
    {
        *slist = curl_slist_append(*slist, String_val(Field(option, 0)));

        option = Field(option, 1);
    }

    result = curl_easy_setopt(conn->connection, curl_option, *slist);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

#define SETOPT_STRING(name) \
static void handle_##name(Connection *conn, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLE_OK; \
\
    Store_field(conn->ocamlValues, Ocaml_##name, option); \
\
    if (conn->curl_##name != NULL) \
        free(conn->curl_##name); \
\
    conn->curl_##name = strdup(String_val(option)); \
\
    result = curl_easy_setopt(conn->connection, CURLOPT_##name, conn->curl_##name); \
\
    if (result != CURLE_OK) \
        raiseError(conn, result); \
\
    CAMLreturn0; \
}

#define SETOPT_VAL_(func_name, curl_option, conv_val) \
static void func_name(Connection *conn, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLE_OK; \
\
    result = curl_easy_setopt(conn->connection, curl_option, conv_val(option)); \
\
    if (result != CURLE_OK) \
        raiseError(conn, result); \
\
    CAMLreturn0; \
}

#define SETOPT_VAL(name, conv) SETOPT_VAL_(handle_##name, CURLOPT_##name, conv)
#define SETOPT_BOOL(name) SETOPT_VAL(name, Bool_val)
#define SETOPT_LONG(name) SETOPT_VAL(name, Long_val)
#define SETOPT_INT64(name) SETOPT_VAL(name, Int64_val)

#define SETOPT_SLIST(name) \
static void handle_##name(Connection* conn, value option) \
{ \
  handle_slist(conn,&(conn->curl_##name),Ocaml_##name,CURLOPT_##name,option); \
}

SETOPT_STRING( URL)
SETOPT_LONG( INFILESIZE)
SETOPT_STRING( PROXY)
SETOPT_LONG( PROXYPORT)
SETOPT_BOOL( HTTPPROXYTUNNEL)
SETOPT_BOOL( VERBOSE)
SETOPT_BOOL( HEADER)
SETOPT_BOOL( NOPROGRESS)

#if HAVE_DECL_CURLOPT_NOSIGNAL
SETOPT_BOOL( NOSIGNAL)
#endif

SETOPT_BOOL( NOBODY)
SETOPT_BOOL( FAILONERROR)
SETOPT_BOOL( UPLOAD)
SETOPT_BOOL( POST)
SETOPT_BOOL( FTPLISTONLY)
SETOPT_BOOL( FTPAPPEND)


static void handle_NETRC(Connection *conn, value option)
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

#if HAVE_DECL_CURLOPT_ENCODING
static void handle_ENCODING(Connection *conn, value option)
{
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
}
#endif


SETOPT_BOOL( FOLLOWLOCATION)
SETOPT_BOOL( TRANSFERTEXT)
SETOPT_BOOL( PUT)
SETOPT_STRING( USERPWD)
SETOPT_STRING( PROXYUSERPWD)
SETOPT_STRING( RANGE)

static void handle_ERRORBUFFER(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, Ocaml_ERRORBUFFER, option);

    if (conn->curl_ERRORBUFFER != NULL)
        free(conn->curl_ERRORBUFFER);

    conn->curl_ERRORBUFFER = malloc(sizeof(char) * CURL_ERROR_SIZE);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_ERRORBUFFER,
                              conn->curl_ERRORBUFFER);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

SETOPT_LONG( TIMEOUT)

static void handle_POSTFIELDS(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    Store_field(conn->ocamlValues, Ocaml_POSTFIELDS, option);

    if (conn->curl_POSTFIELDS != NULL)
        free(conn->curl_POSTFIELDS);

    conn->curl_POSTFIELDS = strdup_ml(option);

    result = curl_easy_setopt(conn->connection,
                              CURLOPT_POSTFIELDS,
                              conn->curl_POSTFIELDS);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

SETOPT_LONG( POSTFIELDSIZE)
SETOPT_STRING( REFERER)
SETOPT_STRING( USERAGENT)
SETOPT_STRING( FTPPORT)
SETOPT_LONG( LOW_SPEED_LIMIT)
SETOPT_LONG( LOW_SPEED_TIME)
SETOPT_LONG( RESUME_FROM)
SETOPT_STRING( COOKIE)

SETOPT_SLIST( HTTPHEADER)

static void handle_HTTPPOST(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal3(listIter, formItem, contentType);
    CURLcode result = CURLE_OK;

    listIter = option;

    Store_field(conn->ocamlValues, Ocaml_HTTPPOST, option);

    free_curl_slist(conn->httpPostBuffers);
    if (conn->httpPostFirst != NULL)
        curl_formfree(conn->httpPostFirst);

    conn->httpPostBuffers = NULL;
    conn->httpPostFirst = NULL;
    conn->httpPostLast = NULL;

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
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_COPYCONTENTS,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTSLENGTH,
                             string_length(Field(formItem, 1)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                contentType = Field(formItem, 2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_COPYCONTENTS,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTSLENGTH,
                             string_length(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
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
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILECONTENT,
                             String_val(Field(formItem, 1)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                contentType = Field(formItem, 2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILECONTENT,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
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
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILE,
                             String_val(Field(formItem, 1)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 2)))
            {
                contentType = Field(formItem, 2);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_FILE,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
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
                conn->httpPostBuffers = curl_slist_prepend_ml(conn->httpPostBuffers, Field(formItem, 2));

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             String_val(Field(formItem, 1)),
                             CURLFORM_BUFFERPTR,
                             conn->httpPostBuffers->data,
                             CURLFORM_BUFFERLENGTH,
                             string_length(Field(formItem, 2)),
                             CURLFORM_END);
            }
            else if (Is_block(Field(formItem, 3)))
            {
                conn->httpPostBuffers = curl_slist_prepend_ml(conn->httpPostBuffers, Field(formItem, 2));

                contentType = Field(formItem, 3);

                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             String_val(Field(formItem, 1)),
                             CURLFORM_BUFFERPTR,
                             conn->httpPostBuffers->data,
                             CURLFORM_BUFFERLENGTH,
                             string_length(Field(formItem, 2)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
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

SETOPT_STRING( SSLCERT)
SETOPT_STRING( SSLCERTTYPE)
SETOPT_STRING( SSLCERTPASSWD)
SETOPT_STRING( SSLKEY)
SETOPT_STRING( SSLKEYTYPE)
SETOPT_STRING( SSLKEYPASSWD)
SETOPT_STRING( SSLENGINE)
SETOPT_BOOL( SSLENGINE_DEFAULT)
SETOPT_BOOL( CRLF)

SETOPT_SLIST( QUOTE)
SETOPT_SLIST( POSTQUOTE)

SETOPT_STRING( COOKIEFILE)
SETOPT_LONG( SSLVERSION)

static void handle_TIMECONDITION(Connection *conn, value option)
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

SETOPT_VAL( TIMEVALUE, Int32_val)
SETOPT_STRING( CUSTOMREQUEST)
SETOPT_STRING( INTERFACE)

static void handle_KRB4LEVEL(Connection *conn, value option)
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

SETOPT_BOOL( SSL_VERIFYPEER)
SETOPT_STRING( CAINFO)
SETOPT_STRING( CAPATH)
SETOPT_BOOL( FILETIME)
SETOPT_LONG( MAXREDIRS)
SETOPT_LONG( MAXCONNECTS)

static void handle_CLOSEPOLICY(Connection *conn, value option)
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

SETOPT_BOOL( FRESH_CONNECT)
SETOPT_BOOL( FORBID_REUSE)
SETOPT_STRING( RANDOM_FILE)
SETOPT_STRING( EGDSOCKET)
SETOPT_LONG( CONNECTTIMEOUT)
SETOPT_BOOL( HTTPGET)

static void handle_SSL_VERIFYHOST(Connection *conn, value option)
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

SETOPT_STRING( COOKIEJAR)
SETOPT_STRING( SSL_CIPHER_LIST)

static void handle_HTTP_VERSION(Connection *conn, value option)
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

SETOPT_BOOL( FTP_USE_EPSV)
SETOPT_LONG( DNS_CACHE_TIMEOUT)
SETOPT_BOOL( DNS_USE_GLOBAL_CACHE)

#if HAVE_DECL_CURLOPT_PRIVATE
SETOPT_STRING( PRIVATE)
#endif

#if HAVE_DECL_CURLOPT_HTTP200ALIASES
SETOPT_SLIST( HTTP200ALIASES)
#endif

#if HAVE_DECL_CURLOPT_UNRESTRICTED_AUTH
SETOPT_BOOL( UNRESTRICTED_AUTH)
#endif

#if HAVE_DECL_CURLOPT_FTP_USE_EPRT
SETOPT_BOOL( FTP_USE_EPRT)
#endif

#if HAVE_DECL_CURLOPT_HTTPAUTH
static void handle_HTTPAUTH(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_FTP_CREATE_MISSING_DIRS
SETOPT_BOOL( FTP_CREATE_MISSING_DIRS)
#endif

#if HAVE_DECL_CURLOPT_PROXYAUTH
static void handle_PROXYAUTH(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_FTP_RESPONSE_TIMEOUT
SETOPT_LONG( FTP_RESPONSE_TIMEOUT)
#endif

#if HAVE_DECL_CURLOPT_IPRESOLVE
static void handle_IPRESOLVE(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_MAXFILESIZE
SETOPT_VAL( MAXFILESIZE, Int32_val)
#endif

#if HAVE_DECL_CURLOPT_INFILESIZE_LARGE
SETOPT_INT64( INFILESIZE_LARGE)
#endif

#if HAVE_DECL_CURLOPT_RESUME_FROM_LARGE
SETOPT_INT64( RESUME_FROM_LARGE)
#endif

#if HAVE_DECL_CURLOPT_MAXFILESIZE_LARGE
SETOPT_INT64( MAXFILESIZE_LARGE)
#endif

#if HAVE_DECL_CURLOPT_NETRC_FILE
SETOPT_STRING( NETRC_FILE)
#endif

#if HAVE_DECL_CURLOPT_FTP_SSL
static void handle_FTP_SSL(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_POSTFIELDSIZE_LARGE
SETOPT_INT64( POSTFIELDSIZE_LARGE)
#endif

#if HAVE_DECL_CURLOPT_TCP_NODELAY
/* not using SETOPT_BOOL here because of TCP_NODELAY defined in winsock.h */
SETOPT_VAL_( handle_TCP_NODELAY, CURLOPT_TCP_NODELAY, Bool_val)
#endif

#if HAVE_DECL_CURLOPT_FTPSSLAUTH
static void handle_FTPSSLAUTH(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_FTP_ACCOUNT
SETOPT_STRING( FTP_ACCOUNT)
#endif

#if HAVE_DECL_CURLOPT_COOKIELIST
SETOPT_STRING( COOKIELIST)
#endif

#if HAVE_DECL_CURLOPT_IGNORE_CONTENT_LENGTH
SETOPT_BOOL( IGNORE_CONTENT_LENGTH)
#endif

#if HAVE_DECL_CURLOPT_FTP_SKIP_PASV_IP
SETOPT_BOOL( FTP_SKIP_PASV_IP)
#endif

#if HAVE_DECL_CURLOPT_FTP_FILEMETHOD
static void handle_FTP_FILEMETHOD(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_LOCALPORT
SETOPT_LONG( LOCALPORT)
#endif

#if HAVE_DECL_CURLOPT_LOCALPORTRANGE
SETOPT_LONG( LOCALPORTRANGE)
#endif

#if HAVE_DECL_CURLOPT_CONNECT_ONLY
SETOPT_BOOL( CONNECT_ONLY)
#endif

#if HAVE_DECL_CURLOPT_MAX_SEND_SPEED_LARGE
SETOPT_INT64( MAX_SEND_SPEED_LARGE)
#endif

#if HAVE_DECL_CURLOPT_MAX_RECV_SPEED_LARGE
SETOPT_INT64( MAX_RECV_SPEED_LARGE)
#endif

#if HAVE_DECL_CURLOPT_FTP_ALTERNATIVE_TO_USER
SETOPT_STRING( FTP_ALTERNATIVE_TO_USER)
#endif

#if HAVE_DECL_CURLOPT_SSL_SESSIONID_CACHE
SETOPT_BOOL( SSL_SESSIONID_CACHE)
#endif

#if HAVE_DECL_CURLOPT_SSH_AUTH_TYPES
static void handle_SSH_AUTH_TYPES(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEYFILE
SETOPT_STRING( SSH_PUBLIC_KEYFILE)
#endif

#if HAVE_DECL_CURLOPT_SSH_PRIVATE_KEYFILE
SETOPT_STRING( SSH_PRIVATE_KEYFILE)
#endif

#if HAVE_DECL_CURLOPT_FTP_SSL_CCC
static void handle_FTP_SSL_CCC(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_TIMEOUT_MS
SETOPT_LONG( TIMEOUT_MS)
#endif

#if HAVE_DECL_CURLOPT_CONNECTTIMEOUT_MS
SETOPT_LONG( CONNECTTIMEOUT_MS)
#endif

#if HAVE_DECL_CURLOPT_HTTP_TRANSFER_DECODING
SETOPT_BOOL( HTTP_TRANSFER_DECODING)
#endif

#if HAVE_DECL_CURLOPT_HTTP_CONTENT_DECODING
SETOPT_BOOL( HTTP_CONTENT_DECODING)
#endif

#if HAVE_DECL_CURLOPT_NEW_FILE_PERMS
SETOPT_LONG( NEW_FILE_PERMS)
#endif

#if HAVE_DECL_CURLOPT_NEW_DIRECTORY_PERMS
SETOPT_LONG( NEW_DIRECTORY_PERMS)
#endif

#if HAVE_DECL_CURLOPT_POST301
SETOPT_BOOL( POST301)
#endif

#if HAVE_DECL_CURLOPT_SSH_HOST_PUBLIC_KEY_MD5
SETOPT_STRING( SSH_HOST_PUBLIC_KEY_MD5)
#endif

#if HAVE_DECL_CURLOPT_COPYPOSTFIELDS
SETOPT_STRING( COPYPOSTFIELDS)
#endif

#if HAVE_DECL_CURLOPT_PROXY_TRANSFER_MODE
SETOPT_BOOL( PROXY_TRANSFER_MODE)
#endif

#if HAVE_DECL_CURLOPT_AUTOREFERER
SETOPT_BOOL( AUTOREFERER)
#endif

#if HAVE_DECL_CURLOPT_PROXYTYPE
static void handle_PROXYTYPE(Connection *conn, value option)
{
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
}
#endif

#if HAVE_DECL_CURLOPT_PROTOCOLS || HAVE_DECL_CURLOPT_REDIR_PROTOCOLS

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

static void handle_PROTOCOLSOPTION(CURLoption curlopt, Connection *conn, value option)
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
#endif

#if HAVE_DECL_CURLOPT_PROTOCOLS
static void handle_PROTOCOLS(Connection *conn, value option)
{
  handle_PROTOCOLSOPTION(CURLOPT_PROTOCOLS, conn, option);
}
#endif

#if HAVE_DECL_CURLOPT_REDIR_PROTOCOLS
static void handle_REDIR_PROTOCOLS(Connection *conn, value option)
{
  handle_PROTOCOLSOPTION(CURLOPT_REDIR_PROTOCOLS, conn, option);
}
#endif

#if HAVE_DECL_CURLOPT_RESOLVE
SETOPT_SLIST( RESOLVE)
#endif

#if HAVE_DECL_CURLOPT_DNS_SERVERS
SETOPT_STRING( DNS_SERVERS)
#endif

#if HAVE_DECL_CURLOPT_MAIL_FROM
SETOPT_STRING( MAIL_FROM)
#endif

#if HAVE_DECL_CURLOPT_MAIL_RCPT
SETOPT_SLIST( MAIL_RCPT)
#endif

/**
 **  curl_easy_setopt helper function
 **/

#define MAP(name) { handle_ ## name, "CURLOPT_"#name, Ocaml_##name }
#define MAP_NO(name) { NULL, "CURLOPT_"#name , Ocaml_##name }
#define IMM(name) { handle_ ## name, "CURLOPT_"#name, -1 }
#define IMM_NO(name) { NULL, "CURLOPT_"#name , -1 }

CURLOptionMapping implementedOptionMap[] =
{
  MAP(WRITEFUNCTION),
  MAP(READFUNCTION),
  IMM(INFILESIZE),
  MAP(URL),
  MAP(PROXY),
  IMM(PROXYPORT),
  IMM(HTTPPROXYTUNNEL),
  IMM(VERBOSE),
  IMM(HEADER),
  IMM(NOPROGRESS),
#if HAVE_DECL_CURLOPT_NOSIGNAL
  IMM(NOSIGNAL),
#else
  IMM_NO(NOSIGNAL),
#endif
  IMM(NOBODY),
  IMM(FAILONERROR),
  IMM(UPLOAD),
  IMM(POST),
  IMM(FTPLISTONLY),
  IMM(FTPAPPEND),
  IMM(NETRC),
#if HAVE_DECL_CURLOPT_ENCODING
  IMM(ENCODING),
#else
  IMM_NO(ENCODING),
#endif
  IMM(FOLLOWLOCATION),
  IMM(TRANSFERTEXT),
  IMM(PUT),
  MAP(USERPWD),
  MAP(PROXYUSERPWD),
  MAP(RANGE),
  IMM(ERRORBUFFER), /* mutable buffer, as output value, do not duplicate */
  IMM(TIMEOUT),
  MAP(POSTFIELDS),
  IMM(POSTFIELDSIZE),
  MAP(REFERER),
  MAP(USERAGENT),
  MAP(FTPPORT),
  IMM(LOW_SPEED_LIMIT),
  IMM(LOW_SPEED_TIME),
  IMM(RESUME_FROM),
  MAP(COOKIE),
  MAP(HTTPHEADER),
  MAP(HTTPPOST),
  MAP(SSLCERT),
  MAP(SSLCERTTYPE),
  MAP(SSLCERTPASSWD),
  MAP(SSLKEY),
  MAP(SSLKEYTYPE),
  MAP(SSLKEYPASSWD),
  MAP(SSLENGINE),
  IMM(SSLENGINE_DEFAULT),
  IMM(CRLF),
  MAP(QUOTE),
  MAP(POSTQUOTE),
  MAP(HEADERFUNCTION),
  MAP(COOKIEFILE),
  IMM(SSLVERSION),
  IMM(TIMECONDITION),
  IMM(TIMEVALUE),
  MAP(CUSTOMREQUEST),
  MAP(INTERFACE),
  IMM(KRB4LEVEL),
  MAP(PROGRESSFUNCTION),
  IMM(SSL_VERIFYPEER),
  MAP(CAINFO),
  MAP(CAPATH),
  IMM(FILETIME),
  IMM(MAXREDIRS),
  IMM(MAXCONNECTS),
  IMM(CLOSEPOLICY),
  IMM(FRESH_CONNECT),
  IMM(FORBID_REUSE),
  MAP(RANDOM_FILE),
  MAP(EGDSOCKET),
  IMM(CONNECTTIMEOUT),
  IMM(HTTPGET),
  IMM(SSL_VERIFYHOST),
  MAP(COOKIEJAR),
  MAP(SSL_CIPHER_LIST),
  IMM(HTTP_VERSION),
  IMM(FTP_USE_EPSV),
  IMM(DNS_CACHE_TIMEOUT),
  IMM(DNS_USE_GLOBAL_CACHE),
  MAP(DEBUGFUNCTION),
#if HAVE_DECL_CURLOPT_PRIVATE
  MAP(PRIVATE),
#else
  MAP_NO(PRIVATE),
#endif
#if HAVE_DECL_CURLOPT_HTTP200ALIASES
  MAP(HTTP200ALIASES),
#else
  MAP_NO(HTTP200ALIASES),
#endif
#if HAVE_DECL_CURLOPT_UNRESTRICTED_AUTH
  IMM(UNRESTRICTED_AUTH),
#else
  IMM_NO(UNRESTRICTED_AUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_USE_EPRT
  IMM(FTP_USE_EPRT),
#else
  IMM_NO(FTP_USE_EPRT),
#endif
#if HAVE_DECL_CURLOPT_HTTPAUTH
  IMM(HTTPAUTH),
#else
  IMM_NO(HTTPAUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_CREATE_MISSING_DIRS
  IMM(FTP_CREATE_MISSING_DIRS),
#else
  IMM_NO(FTP_CREATE_MISSING_DIRS),
#endif
#if HAVE_DECL_CURLOPT_PROXYAUTH
  IMM(PROXYAUTH),
#else
  IMM_NO(PROXYAUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_RESPONSE_TIMEOUT
  IMM(FTP_RESPONSE_TIMEOUT),
#else
  IMM_NO(FTP_RESPONSE_TIMEOUT),
#endif
#if HAVE_DECL_CURLOPT_IPRESOLVE
  IMM(IPRESOLVE),
#else
  IMM_NO(IPRESOLVE),
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE
  IMM(MAXFILESIZE),
#else
  IMM_NO(MAXFILESIZE),
#endif
#if HAVE_DECL_CURLOPT_INFILESIZE_LARGE
  IMM(INFILESIZE_LARGE),
#else
  IMM_NO(INFILESIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_RESUME_FROM_LARGE
  IMM(RESUME_FROM_LARGE),
#else
  IMM_NO(RESUME_FROM_LARGE),
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE_LARGE
  IMM(MAXFILESIZE_LARGE),
#else
  IMM_NO(MAXFILESIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_NETRC_FILE
  MAP(NETRC_FILE),
#else
  MAP_NO(NETRC_FILE),
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL
  IMM(FTP_SSL),
#else
  IMM_NO(FTP_SSL),
#endif
#if HAVE_DECL_CURLOPT_POSTFIELDSIZE_LARGE
  IMM(POSTFIELDSIZE_LARGE),
#else
  IMM_NO(POSTFIELDSIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_TCP_NODELAY
  IMM(TCP_NODELAY),
#else
  IMM_NO(TCP_NODELAY),
#endif
#if HAVE_DECL_CURLOPT_FTPSSLAUTH
  IMM(FTPSSLAUTH),
#else
  IMM_NO(FTPSSLAUTH),
#endif
#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
  MAP(IOCTLFUNCTION),
#else
  MAP_NO(IOCTLFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_FTP_ACCOUNT
  MAP(FTP_ACCOUNT),
#else
  MAP_NO(FTP_ACCOUNT),
#endif
#if HAVE_DECL_CURLOPT_COOKIELIST
  MAP(COOKIELIST),
#else
  MAP_NO(COOKIELIST),
#endif
#if HAVE_DECL_CURLOPT_IGNORE_CONTENT_LENGTH
  IMM(IGNORE_CONTENT_LENGTH),
#else
  IMM_NO(IGNORE_CONTENT_LENGTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_SKIP_PASV_IP
  IMM(FTP_SKIP_PASV_IP),
#else
  IMM_NO(FTP_SKIP_PASV_IP),
#endif
#if HAVE_DECL_CURLOPT_FTP_FILEMETHOD
  IMM(FTP_FILEMETHOD),
#else
  IMM_NO(FTP_FILEMETHOD),
#endif
#if HAVE_DECL_CURLOPT_LOCALPORT
  IMM(LOCALPORT),
#else
  IMM_NO(LOCALPORT),
#endif
#if HAVE_DECL_CURLOPT_LOCALPORTRANGE
  IMM(LOCALPORTRANGE),
#else
  IMM_NO(LOCALPORTRANGE),
#endif
#if HAVE_DECL_CURLOPT_CONNECT_ONLY
  IMM(CONNECT_ONLY),
#else
  IMM_NO(CONNECT_ONLY),
#endif
#if HAVE_DECL_CURLOPT_MAX_SEND_SPEED_LARGE
  IMM(MAX_SEND_SPEED_LARGE),
#else
  IMM_NO(MAX_SEND_SPEED_LARGE),
#endif
#if HAVE_DECL_CURLOPT_MAX_RECV_SPEED_LARGE
  IMM(MAX_RECV_SPEED_LARGE),
#else
  IMM_NO(MAX_RECV_SPEED_LARGE),
#endif
#if HAVE_DECL_CURLOPT_FTP_ALTERNATIVE_TO_USER
  MAP(FTP_ALTERNATIVE_TO_USER),
#else
  MAP_NO(FTP_ALTERNATIVE_TO_USER),
#endif
#if HAVE_DECL_CURLOPT_SSL_SESSIONID_CACHE
  IMM(SSL_SESSIONID_CACHE),
#else
  IMM_NO(SSL_SESSIONID_CACHE),
#endif
#if HAVE_DECL_CURLOPT_SSH_AUTH_TYPES
  IMM(SSH_AUTH_TYPES),
#else
  IMM_NO(SSH_AUTH_TYPES),
#endif
#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEYFILE
  MAP(SSH_PUBLIC_KEYFILE),
#else
  MAP_NO(SSH_PUBLIC_KEYFILE),
#endif
#if HAVE_DECL_CURLOPT_SSH_PRIVATE_KEYFILE
  MAP(SSH_PRIVATE_KEYFILE),
#else
  MAP_NO(SSH_PRIVATE_KEYFILE),
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL_CCC
  IMM(FTP_SSL_CCC),
#else
  IMM_NO(FTP_SSL_CCC),
#endif
#if HAVE_DECL_CURLOPT_TIMEOUT_MS
  IMM(TIMEOUT_MS),
#else
  IMM_NO(TIMEOUT_MS),
#endif
#if HAVE_DECL_CURLOPT_CONNECTTIMEOUT_MS
  IMM(CONNECTTIMEOUT_MS),
#else
  IMM_NO(CONNECTTIMEOUT_MS),
#endif
#if HAVE_DECL_CURLOPT_HTTP_TRANSFER_DECODING
  IMM(HTTP_TRANSFER_DECODING),
#else
  IMM_NO(HTTP_TRANSFER_DECODING),
#endif
#if HAVE_DECL_CURLOPT_HTTP_CONTENT_DECODING
  IMM(HTTP_CONTENT_DECODING),
#else
  IMM_NO(HTTP_CONTENT_DECODING),
#endif
#if HAVE_DECL_CURLOPT_NEW_FILE_PERMS
  IMM(NEW_FILE_PERMS),
#else
  IMM_NO(NEW_FILE_PERMS),
#endif
#if HAVE_DECL_CURLOPT_NEW_DIRECTORY_PERMS
  IMM(NEW_DIRECTORY_PERMS),
#else
  IMM_NO(NEW_DIRECTORY_PERMS),
#endif
#if HAVE_DECL_CURLOPT_POST301
  IMM(POST301),
#else
  IMM_NO(POST301),
#endif
#if HAVE_DECL_CURLOPT_SSH_HOST_PUBLIC_KEY_MD5
  MAP(SSH_HOST_PUBLIC_KEY_MD5),
#else
  MAP_NO(SSH_HOST_PUBLIC_KEY_MD5),
#endif
#if HAVE_DECL_CURLOPT_COPYPOSTFIELDS
  MAP(COPYPOSTFIELDS),
#else
  MAP_NO(COPYPOSTFIELDS),
#endif
#if HAVE_DECL_CURLOPT_PROXY_TRANSFER_MODE
  IMM(PROXY_TRANSFER_MODE),
#else
  IMM_NO(PROXY_TRANSFER_MODE),
#endif
#if HAVE_DECL_CURLOPT_SEEKFUNCTION
  MAP(SEEKFUNCTION),
#else
  MAP_NO(SEEKFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_AUTOREFERER
  IMM(AUTOREFERER),
#else
  IMM_NO(AUTOREFERER),
#endif
#if HAVE_DECL_CURLOPT_OPENSOCKETFUNCTION
  MAP(OPENSOCKETFUNCTION),
#else
  MAP_NO(OPENSOCKETFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_PROXYTYPE
  IMM(PROXYTYPE),
#else
  IMM_NO(PROXYTYPE),
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS
  IMM(PROTOCOLS),
#else
  IMM_NO(PROTOCOLS),
#endif
#if HAVE_DECL_CURLOPT_REDIR_PROTOCOLS
  IMM(REDIR_PROTOCOLS),
#else
  IMM_NO(REDIR_PROTOCOLS),
#endif
#if HAVE_DECL_CURLOPT_RESOLVE
  MAP(RESOLVE),
#else
  MAP_NO(RESOLVE),
#endif
#if HAVE_DECL_CURLOPT_DNS_SERVERS
  MAP(DNS_SERVERS),
#else
  MAP_NO(DNS_SERVERS),
#endif
#if HAVE_DECL_CURLOPT_MAIL_FROM
  MAP(MAIL_FROM),
#else
  MAP_NO(MAIL_FROM),
#endif
#if HAVE_DECL_CURLOPT_MAIL_RCPT
  MAP(MAIL_RCPT),
#else
  MAP_NO(MAIL_RCPT),
#endif
};

static Connection *duplicateConnection(Connection *original)
{
    Connection *connection = NULL;
    CURL* h = NULL;
    size_t i = 0;
    CURLOptionMapping* this = NULL;

    caml_enter_blocking_section();
    h  = curl_easy_duphandle(original->connection);
    caml_leave_blocking_section();

    connection = allocConnection(h);

    for (i = 0; i < sizeof(implementedOptionMap)/sizeof(CURLOptionMapping); i++)
    {
      this = &implementedOptionMap[i];
      if (-1 == this->ocamlValue) continue;
      if (this->optionHandler && (Field(original->ocamlValues, this->ocamlValue) != Val_unit))
      {
        this->optionHandler(connection, Field(original->ocamlValues, this->ocamlValue));
      }
    }

    return connection;
}

CAMLprim value helper_curl_easy_setopt(value conn, value option)
{
    CAMLparam2(conn, option);
    CAMLlocal1(data);
    Connection *connection = Connection_val(conn);
    CURLOptionMapping* thisOption = NULL;
    static value* exception = NULL;

    checkConnection(connection);

    data = Field(option, 0);

    if (Tag_val(option) < sizeof(implementedOptionMap)/sizeof(CURLOptionMapping))
    {
      thisOption = &implementedOptionMap[Tag_val(option)];
      if (thisOption->optionHandler)
      {
        thisOption->optionHandler(connection, data);
      }
      else
      {
        if (NULL == exception)
        {
          exception = caml_named_value("Curl.NotImplemented");
          if (NULL == exception) caml_invalid_argument("Curl.NotImplemented");
        }

        caml_raise_with_string(*exception, thisOption->name);
      }
    }
    else
    {
      caml_failwith("Invalid CURLOPT Option");
    }

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

  caml_stat_free(h);
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
    if (conn->curl_ERRORBUFFER != NULL)
    {
        Store_field(Field(conn->ocamlValues, Ocaml_ERRORBUFFER), 0, caml_copy_string(conn->curl_ERRORBUFFER));
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

static void raise_multi_error(char const* msg)
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
  raise_multi_error(s);
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
      raise_multi_error("caml_curl_multi_socket_action");
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
      raise_multi_error("curlm_sock_cb"); /* FIXME exception from callback */
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

#define SETMOPT_VAL_(func_name, curl_option, conv_val) \
static void func_name(CURLM *handle, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLM_OK; \
\
    result = curl_multi_setopt(handle, curl_option, conv_val(option)); \
\
    check_mcode(result); \
\
    CAMLreturn0; \
}

#define SETMOPT_VAL(name, conv) SETMOPT_VAL_(handle_multi_##name, CURLMOPT_##name, conv)
#define SETMOPT_BOOL(name) SETMOPT_VAL(name, Bool_val)
#define SETMOPT_LONG(name) SETMOPT_VAL(name, Long_val)
#define SETMOPT_INT64(name) SETMOPT_VAL(name, Int64_val)

SETMOPT_BOOL( PIPELINING)

#if HAVE_DECL_CURLMOPT_MAXCONNECTS
SETMOPT_LONG( MAXCONNECTS)
#endif

#if HAVE_DECL_CURLMOPT_MAX_PIPELINE_LENGTH
SETMOPT_LONG( MAX_PIPELINE_LENGTH)
#endif

#if HAVE_DECL_CURLMOPT_MAX_HOST_CONNECTIONS
SETMOPT_LONG( MAX_HOST_CONNECTIONS)
#endif

typedef struct CURLMOptionMapping CURLMOptionMapping;
#define OPT(name) { handle_multi_## name, "CURLMOPT_"#name}
#define NO_OPT(name) { NULL, "CURLMOPT_"#name}

struct CURLMOptionMapping
{
    void (*optionHandler)(CURLM *, value);
    char *name;
};

CURLMOptionMapping implementedMOptionMap[] = {
  OPT( PIPELINING),
#if HAVE_DECL_CURLMOPT_MAXCONNECTS
  OPT( MAXCONNECTS),
#else
  NO_OPT( MAXCONNECTS),
#endif
#if HAVE_DECL_CURLMOPT_MAX_PIPELINE_LENGTH
  OPT( MAX_PIPELINE_LENGTH),
#else
  NO_OPT( MAX_PIPELINE_LENGTH),
#endif
#if HAVE_DECL_CURLMOPT_MAX_HOST_CONNECTIONS
  OPT( MAX_HOST_CONNECTIONS),
#else
  NO_OPT( MAX_HOST_CONNECTIONS),
#endif
};

CAMLprim value caml_curl_multi_setopt(value v_multi, value option)
{
    CAMLparam2(v_multi, option);
    CAMLlocal1(data);
    CURLM *handle = Multi_val(v_multi)->handle;
    CURLMOptionMapping* thisOption = NULL;
    static value* exception = NULL;

    data = Field(option, 0);

    if (Tag_val(option) < sizeof(implementedMOptionMap)/sizeof(CURLMOptionMapping))
    {
      thisOption = &implementedMOptionMap[Tag_val(option)];
      if (thisOption->optionHandler)
      {
        thisOption->optionHandler(handle, data);
      }
      else
      {
        if (NULL == exception)
        {
          exception = caml_named_value("Curl.NotImplemented");
          if (NULL == exception) caml_invalid_argument("Curl.NotImplemented");
        }

        caml_raise_with_string(*exception, thisOption->name);
      }
    }
    else
    {
      caml_failwith("Invalid CURLMOPT Option");
    }

    CAMLreturn(Val_unit);
}

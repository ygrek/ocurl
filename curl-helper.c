/***
 ***  curl-helper.c
 ***
 ***  Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 ***  Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 ***/

#ifndef CAML_NAME_SPACE
#define CAML_NAME_SPACE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <caml/config.h> /* defines HAS_UNISTD */
#ifdef HAS_UNISTD
#include <unistd.h>
#endif
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
#include <caml/threads.h>

#ifndef CAMLdrop
#define CAMLdrop caml_local_roots = caml__frame
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#pragma message("No config file given.")
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

/*
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

typedef struct Connection Connection;
typedef struct ConnectionList ConnectionList;

#define Connection_val(v) (*(Connection**)Data_custom_val(v))

typedef enum OcamlValues
{
    Ocaml_WRITEFUNCTION,
    Ocaml_READFUNCTION,
    Ocaml_HEADERFUNCTION,
    Ocaml_PROGRESSFUNCTION,
    Ocaml_XFERINFOFUNCTION,
    Ocaml_PREREQFUNCTION,
    Ocaml_DEBUGFUNCTION,
    Ocaml_IOCTLFUNCTION,
    Ocaml_SEEKFUNCTION,
    Ocaml_OPENSOCKETFUNCTION,
    /* Ocaml_CLOSESOCKETFUNCTION, */
    Ocaml_SSH_KEYFUNCTION,

    Ocaml_ERRORBUFFER,
    Ocaml_PRIVATE,

    /* Not used, last for size */
    OcamlValuesSize
} OcamlValue;

struct Connection
{
    CURL *handle;

    value ocamlValues;

    size_t refcount; /* number of references to this structure */

    char *curl_ERRORBUFFER;
    char *curl_POSTFIELDS;
    struct curl_slist *curl_HTTPHEADER;
    struct curl_slist *httpPostBuffers;
    struct curl_httppost *httpPostFirst;
    struct curl_httppost *httpPostLast;
    struct curl_slist *curl_RESOLVE;
    struct curl_slist *curl_QUOTE;
    struct curl_slist *curl_POSTQUOTE;
    struct curl_slist *curl_HTTP200ALIASES;
    struct curl_slist *curl_MAIL_RCPT;
    struct curl_slist *curl_CONNECT_TO;
#if HAVE_DECL_CURLOPT_MIMEPOST
    curl_mime *curl_MIMEPOST;
#endif
};

typedef struct CURLErrorMapping CURLErrorMapping;

struct CURLErrorMapping
{
    char *name;
    CURLcode error;
};

CURLErrorMapping errorMap[] =
{
    {"CURLE_OK", CURLE_OK},
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
#if HAVE_DECL_CURLE_HTTP_RETURNED_ERROR
    {"CURLE_HTTP_RETURNED_ERROR", CURLE_HTTP_RETURNED_ERROR},
#else
    {"CURLE_HTTP_RETURNED_ERROR", -1},
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
#if HAVE_DECL_CURLE_BAD_DOWNLOAD_RESUME
    {"CURLE_BAD_DOWNLOAD_RESUME", CURLE_BAD_DOWNLOAD_RESUME},
#else
    {"CURLE_BAD_DOWNLOAD_RESUME", -1},
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
#if HAVE_DECL_CURLE_INTERFACE_FAILED
    {"CURLE_INTERFACE_FAILED", CURLE_INTERFACE_FAILED},
#else
    {"CURLE_INTERFACE_FAILED", -1},
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
#if HAVE_DECL_CURLE_SSL_ENGINE_NOTFOUND
    {"CURLE_SSL_ENGINE_NOTFOUND", CURLE_SSL_ENGINE_NOTFOUND},
#else
    {"CURLE_SSL_ENGINE_NOTFOUND", -1},
#endif
#if HAVE_DECL_CURLE_SSL_ENGINE_SETFAILED
    {"CURLE_SSL_ENGINE_SETFAILED", CURLE_SSL_ENGINE_SETFAILED},
#else
    {"CURLE_SSL_ENGINE_SETFAILED", -1},
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
    {"CURLE_SSL_CERTPROBLEM", CURLE_SSL_CERTPROBLEM},
#else
    {"CURLE_SSL_CERTPROBLEM", -1},
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
#if HAVE_DECL_CURLE_CONV_REQD
    {"CURLE_CONV_REQD", CURLE_CONV_REQD},
#else
    {"CURLE_CONV_REQD", -1},
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
    /* sync check_enums */
    {NULL, (CURLcode)0}
};

typedef struct CURLOptionMapping CURLOptionMapping;

struct CURLOptionMapping
{
    void (*optionHandler)(Connection *, value);
    char *name;
};

static char* strdup_ml(value v)
{
  char* p = NULL;
  p = (char*)malloc(caml_string_length(v)+1);
  memcpy(p,String_val(v),caml_string_length(v)+1); // caml strings have terminating zero
  return p;
}

static value ml_copy_string(char const* p, size_t size)
{
  value v = caml_alloc_string(size);
  memcpy(&Byte(v,0),p,size);
  return v;
}

/* prepends to the beginning of list */
static struct curl_slist* curl_slist_prepend_ml(struct curl_slist* list, value v)
{
  /* FIXME check NULLs */
  struct curl_slist* new_item = (struct curl_slist*)malloc(sizeof(struct curl_slist));

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
    const value *exception;
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

    exceptionData = caml_alloc_tuple(3);

    Store_field(exceptionData, 0, Val_int(code));
    Store_field(exceptionData, 1, Val_int(code));
    Store_field(exceptionData, 2, caml_copy_string(errorString));

    if (conn != NULL && conn->curl_ERRORBUFFER != NULL)
    {
        Store_field(Field(conn->ocamlValues, Ocaml_ERRORBUFFER), 0, caml_copy_string(conn->curl_ERRORBUFFER));
    }

    exception = caml_named_value("CurlException");

    if (exception == NULL)
        caml_failwith("CurlException not registered");

    caml_raise_with_arg(*exception, exceptionData);

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
    caml_register_global_root(&connection->ocamlValues);

    connection->handle = h;
    curl_easy_setopt(h, CURLOPT_PRIVATE, connection);

    connection->refcount = 0;

    connection->curl_ERRORBUFFER = NULL;
    connection->curl_POSTFIELDS = NULL;
    connection->curl_HTTPHEADER = NULL;
    connection->httpPostBuffers = NULL;
    connection->httpPostFirst = NULL;
    connection->httpPostLast = NULL;
    connection->curl_QUOTE = NULL;
    connection->curl_POSTQUOTE = NULL;
    connection->curl_HTTP200ALIASES = NULL;
    connection->curl_RESOLVE = NULL;
    connection->curl_MAIL_RCPT = NULL;
    connection->curl_CONNECT_TO = NULL;
#if HAVE_DECL_CURLOPT_MIMEPOST
    connection->curl_MIMEPOST = NULL;
#endif

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

    if (!connection->handle)
    {
      return; /* already cleaned up */
    }

    if (finalization)
    {
      /* cannot engage OCaml runtime at finalization, just report leak */
      if (CURLE_OK != curl_easy_getinfo(connection->handle, CURLINFO_EFFECTIVE_URL, &fin_url) || NULL == fin_url)
      {
        fin_url = "unknown";
      }
      fprintf(stderr,"Curl: handle %p leaked, conn %p, url %s\n", connection->handle, connection, fin_url);
      fflush(stderr);
    }
    else
    {
      caml_enter_blocking_section();
      curl_easy_cleanup(connection->handle);
      caml_leave_blocking_section();
    }

    connection->handle = NULL;

    caml_remove_global_root(&connection->ocamlValues);

    free_if(connection->curl_ERRORBUFFER);
    free_if(connection->curl_POSTFIELDS);
    free_curl_slist(connection->curl_HTTPHEADER);
    free_curl_slist(connection->httpPostBuffers);
    if (connection->httpPostFirst != NULL)
        curl_formfree(connection->httpPostFirst);
    free_curl_slist(connection->curl_RESOLVE);
    free_curl_slist(connection->curl_QUOTE);
    free_curl_slist(connection->curl_POSTQUOTE);
    free_curl_slist(connection->curl_HTTP200ALIASES);
    free_curl_slist(connection->curl_MAIL_RCPT);
    free_curl_slist(connection->curl_CONNECT_TO);
#if HAVE_DECL_CURLOPT_MIMEPOST
    curl_mime_free(connection->curl_MIMEPOST);
#endif
}

static Connection* getConnection(CURL* h)
{
  Connection* p = NULL;

  if (CURLE_OK != curl_easy_getinfo(h, CURLINFO_PRIVATE, &p) || NULL == p)
  {
    caml_failwith("Unknown handle");
  }

  return p;
}

#if 1
static void checkConnection(Connection * connection)
{
  (void)connection;
}
#else
static void checkConnection(Connection *connection)
{
  if (connection != getConnection(connection->handle))
  {
    caml_failwith("Invalid Connection");
  }
}
#endif

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

static size_t cb_WRITEFUNCTION(char *ptr, size_t size, size_t nmemb, void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal2(result, str);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    str = ml_copy_string(ptr,size*nmemb);

    result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_WRITEFUNCTION), str);

    size_t r = Is_exception_result(result) ? 0 : Int_val(result);
    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static size_t cb_WRITEFUNCTION2(char *ptr, size_t size, size_t nmemb, void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal2(result, str);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    str = ml_copy_string(ptr,size*nmemb);

    result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_WRITEFUNCTION), str);

    size_t r = 0;

    if (!Is_exception_result(result))
    {
      if (Is_block(result)) /* Proceed */
      {
        r = size * nmemb;
      }
      else
      {
        if (0 == Int_val(result)) /* Pause */
          r = CURL_WRITEFUNC_PAUSE;
        /* else 1 = Abort */
      }
    }

    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static size_t cb_READFUNCTION(void *ptr, size_t size, size_t nmemb, void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    size_t length;

    checkConnection(conn);

    result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_READFUNCTION),
                      Val_int(size*nmemb));

    size_t r = CURL_READFUNC_ABORT;

    if (!Is_exception_result(result))
    {
      length = caml_string_length(result);

      if (length <= size*nmemb)
      {
        memcpy(ptr, String_val(result), length);
        r = length;
      }
    }

    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static size_t cb_READFUNCTION2(void *ptr, size_t size, size_t nmemb, void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    size_t length;

    checkConnection(conn);

    result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_READFUNCTION),
                      Val_int(size*nmemb));

    size_t r = CURL_READFUNC_ABORT;

    if (!Is_exception_result(result))
    {
      if (Is_block(result)) /* Proceed */
      {
        result = Field(result,0);

        length = caml_string_length(result);

        if (length <= size*nmemb)
        {
          memcpy(ptr, String_val(result), length);
          r = length;
        }
      }
      else
      {
        if (0 == Int_val(result)) /* Pause */
          r = CURL_READFUNC_PAUSE;
        /* else 1 = Abort */
      }
    }

    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static size_t cb_HEADERFUNCTION(char *ptr, size_t size, size_t nmemb, void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal2(result,str);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    str = ml_copy_string(ptr,size*nmemb);

    result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_HEADERFUNCTION), str);

    size_t r = Is_exception_result(result) ? 0 : Int_val(result);
    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static int cb_PROGRESSFUNCTION(void *data,
                            double dlTotal,
                            double dlNow,
                            double ulTotal,
                            double ulNow)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    CAMLlocalN(callbackData, 4);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    callbackData[0] = caml_copy_double(dlTotal);
    callbackData[1] = caml_copy_double(dlNow);
    callbackData[2] = caml_copy_double(ulTotal);
    callbackData[3] = caml_copy_double(ulNow);

    result = caml_callbackN_exn(Field(conn->ocamlValues, Ocaml_PROGRESSFUNCTION),
                       4, callbackData);

    int r = Is_exception_result(result) ? 1 : Bool_val(result);
    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

static int cb_XFERINFOFUNCTION(void *data,
                              curl_off_t dlTotal,
                              curl_off_t dlNow,
                              curl_off_t ulTotal,
                              curl_off_t ulNow)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    CAMLlocalN(callbackData, 4);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    callbackData[0] = caml_copy_int64(dlTotal);
    callbackData[1] = caml_copy_int64(dlNow);
    callbackData[2] = caml_copy_int64(ulTotal);
    callbackData[3] = caml_copy_int64(ulNow);

    result = caml_callbackN_exn(Field(conn->ocamlValues, Ocaml_XFERINFOFUNCTION),
                       4, callbackData);

    int r = Is_exception_result(result) ? 1 : Bool_val(result);
    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}

#if HAVE_DECL_CURLOPT_PREREQFUNCTION
static int cb_PREREQFUNCTION(void *data,
                             char *conn_primary_ip,
                             char *conn_local_ip,
                             int conn_primary_port,
                             int conn_local_port)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    CAMLlocalN(callbackData, 4);
    Connection *conn = (Connection *)data;

    checkConnection(conn);

    callbackData[0] = caml_copy_string(conn_primary_ip);
    callbackData[1] = caml_copy_string(conn_local_ip);
    callbackData[2] = Val_int(conn_primary_port);
    callbackData[3] = Val_int(conn_local_port);

    result = caml_callbackN_exn(Field(conn->ocamlValues, Ocaml_PREREQFUNCTION),
                       4, callbackData);

    int r = Is_exception_result(result) ? 1 : Bool_val(result);
    CAMLdrop;

    caml_enter_blocking_section();
    return r;
}
#endif

static int cb_DEBUGFUNCTION(CURL *debugConnection,
                         curl_infotype infoType,
                         char *buffer,
                         size_t bufferLength,
                         void *data)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal3(camlDebugConnection, camlInfoType, camlMessage);
    Connection *conn = (Connection *)data;
    (void)debugConnection; /* not used */

    checkConnection(conn);

    camlDebugConnection = caml_curl_alloc(conn);
    camlMessage = ml_copy_string(buffer,bufferLength);
    camlInfoType = Val_long(infoType <= CURLINFO_SSL_DATA_OUT /* sync check_enums */ ? infoType : CURLINFO_END);

    caml_callback3_exn(Field(conn->ocamlValues, Ocaml_DEBUGFUNCTION),
              camlDebugConnection,
              camlInfoType,
              camlMessage);

    CAMLdrop;

    caml_enter_blocking_section();
    return 0;
}

static curlioerr cb_IOCTLFUNCTION(CURL *ioctl,
                               int cmd,
                               void *data)
{
    caml_leave_blocking_section();

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
        caml_failwith("Invalid IOCTL Cmd!");

    camlConnection = caml_curl_alloc(conn);

    camlResult = caml_callback2_exn(Field(conn->ocamlValues, Ocaml_IOCTLFUNCTION),
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
    CAMLdrop;

    caml_enter_blocking_section();
    return result;
}

#if HAVE_DECL_CURLOPT_SEEKFUNCTION
static int cb_SEEKFUNCTION(void *data,
                        curl_off_t offset,
                        int origin)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal3(camlResult, camlOffset, camlOrigin);
    Connection *conn = (Connection *)data;

    camlOffset = caml_copy_int64(offset);

    if (origin == SEEK_SET)
        camlOrigin = Val_long(0);
    else if (origin == SEEK_CUR)
        camlOrigin = Val_long(1);
    else if (origin == SEEK_END)
        camlOrigin = Val_long(2);
    else
        caml_failwith("Invalid seek code");

    camlResult = caml_callback2_exn(Field(conn->ocamlValues,
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
      default: caml_failwith("Invalid seek result");
    }
    CAMLdrop;

    caml_enter_blocking_section();
    return result;
}
#endif

static int cb_OPENSOCKETFUNCTION(void *data,
                        curlsocktype purpose,
                        struct curl_sockaddr *addr)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(result);
    Connection *conn = (Connection *)data;
    int sock = -1;
    (void)purpose; /* not used */

    sock = socket(addr->family, addr->socktype, addr->protocol);

    if (-1 != sock)
    {
      /* FIXME windows */
      result = caml_callback_exn(Field(conn->ocamlValues, Ocaml_OPENSOCKETFUNCTION), Val_int(sock));
      if (Is_exception_result(result))
      {
        close(sock);
        sock = -1;
      }
    }
    CAMLdrop;

    caml_enter_blocking_section();
    return ((sock == -1) ? CURL_SOCKET_BAD : sock);
}

/*
static int cb_CLOSESOCKETFUNCTION(void *data,
                         curl_socket_t socket)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal1(camlResult);
    Connection *conn = (Connection *)data;
    int result = 0;

    camlResult = caml_callback_exn(Field(conn->ocamlValues, Ocaml_CLOSESOCKETFUNCTION), Val_int(socket));
    if (Is_exception_result(camlResult))
    {
      result = 1;
    }
    CAMLdrop;

    caml_enter_blocking_section();
    return result;
}
*/

static int cb_SSH_KEYFUNCTION(CURL *easy,
                              const struct curl_khkey *knownkey,
                              const struct curl_khkey *foundkey,
                              enum curl_khmatch match,
                              void *clientp)
{
    caml_leave_blocking_section();

    CAMLparam0();
    CAMLlocal3(v_found, v_match, v_result);
    Connection *conn = (Connection *)clientp;
    int res = CURLKHSTAT_REJECT;

    switch (match) {
      case CURLKHMATCH_OK:
        v_match = Val_int(0);
        break;
      case CURLKHMATCH_MISMATCH:
        v_match = caml_alloc_small(1, 0);
        Field(v_match, 0) = ml_copy_string(knownkey->key, knownkey->len ? knownkey->len : strlen(knownkey->key));
        break;
      case CURLKHMATCH_MISSING:
        v_match = Val_int(1);
        break;
      default:
        caml_failwith("Invalid CURL_SSH_KEYFUNCTION argument");
        break;
    }

    v_found = ml_copy_string(foundkey->key, foundkey->len ? foundkey->len : strlen(foundkey->key));
    v_result = caml_callback2_exn(Field(conn->ocamlValues, Ocaml_SSH_KEYFUNCTION), v_match, v_found);

    if (!Is_exception_result(v_result)) {
      switch (Int_val(v_result)) {
        case 0:
          res = CURLKHSTAT_FINE_ADD_TO_FILE;
          break;
        case 1:
          res = CURLKHSTAT_FINE;
          break;
        case 2:
          res = CURLKHSTAT_REJECT;
          break;
        case 3:
          res = CURLKHSTAT_DEFER;
          break;
        default:
          caml_failwith("Invalid CURLOPT_SSH_KEYFUNCTION return value");
          break;
      }
    }

    CAMLdrop;

    caml_enter_blocking_section();
    return res;
}

#if HAVE_DECL_CURL_GLOBAL_SSLSET

/* Same order as in OCaml */
curl_sslbackend sslBackendMap[] = {
#if HAVE_DECL_CURLSSLBACKEND_NONE
  CURLSSLBACKEND_NONE,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_OPENSSL
  CURLSSLBACKEND_OPENSSL,
#else
  -1
#endif
#if HAVE_DECL_CURLSSLBACKEND_GNUTLS
  CURLSSLBACKEND_GNUTLS,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_NSS
  CURLSSLBACKEND_NSS,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_GSKIT
  CURLSSLBACKEND_GSKIT,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_WOLFSSL
  CURLSSLBACKEND_WOLFSSL,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_SCHANNEL
  CURLSSLBACKEND_SCHANNEL,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_SECURETRANSPORT
  CURLSSLBACKEND_SECURETRANSPORT,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_MBEDTLS
  CURLSSLBACKEND_MBEDTLS,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_MESALINK
  CURLSSLBACKEND_MESALINK,
#else
  -1,
#endif
#if HAVE_DECL_CURLSSLBACKEND_BEARSSL
  CURLSSLBACKEND_BEARSSL,
#else
  -1,
#endif
};

/* Same order as in OCaml */
CURLsslset sslsetMap[] = {
  CURLSSLSET_OK,
  CURLSSLSET_UNKNOWN_BACKEND,
  CURLSSLSET_TOO_LATE,
  CURLSSLSET_NO_BACKENDS,
};

static void raiseSslsetError(CURLsslset err)
{
  CAMLparam0();
  const value *exception;
  int i, found;

  for (i = 0, found = -1; i < sizeof(sslsetMap) / sizeof(sslsetMap[0]); i ++) {
    if (sslsetMap[i] == err) {
      found = i;
      break;
    }
  }

  if (found < 0)
    caml_invalid_argument("Unexpected CURLsslset value");

  exception = caml_named_value("CurlSslSetException");
  if (exception == NULL) caml_invalid_argument("CurlSslSetException not registered");

  caml_raise_with_arg(*exception, Val_int(found));

  /* Not reached */
  CAMLreturn0;
}

value caml_curl_global_sslset(value v_backend)
{
  CAMLparam1(v_backend);

  curl_sslbackend backend = sslBackendMap[Int_val(v_backend)];
  CURLsslset res = curl_global_sslset(backend, NULL, NULL);

  if (res != CURLSSLSET_OK)
    raiseSslsetError(res);

  CAMLreturn(Val_unit);
}

value caml_curl_global_sslset_str(value v_backend_str)
{
  CAMLparam1(v_backend_str);

  CURLsslset res = curl_global_sslset(-1, String_val(v_backend_str), NULL);

  if (res != CURLSSLSET_OK)
    raiseSslsetError(res);

  CAMLreturn(Val_unit);
}

value caml_curl_global_sslsetavail(value v_unit)
{
  CAMLparam1(v_unit);
  CAMLlocal1(lst);
  const curl_ssl_backend **backends;
  CURLsslset res;
  int i, j, found;

  res = curl_global_sslset(-1, NULL, &backends);

  if (res != CURLSSLSET_UNKNOWN_BACKEND)
    raiseSslsetError(res);

  lst = Val_emptylist;

  for (i = 0; backends[i] != NULL; i ++) {
    found = -1;

    for (j = 0; j < sizeof(sslBackendMap) / sizeof(sslBackendMap[0]); j ++) {
      if (sslBackendMap[j] == backends[i]->id) {
        found = j;
        break;
      }
    }

    /* If an unknown backend is returned, it is ignored */
    if (found >= 0) {
      lst = Val_cons(lst, Val_long(found));
    }
  }

  CAMLreturn(lst);
}

value caml_curl_global_sslsetavail_str(value v_unit)
{
  CAMLparam1(v_unit);
  CAMLlocal1(lst);
  const curl_ssl_backend **backends;
  CURLsslset res;
  int i;

  res = curl_global_sslset(-1, NULL, &backends);

  if (res != CURLSSLSET_UNKNOWN_BACKEND)
    raiseSslsetError(res);

  lst = Val_emptylist;

  for (i = 0; backends[i] != NULL; i ++) {
    lst = Val_cons(lst, caml_copy_string(backends[i]->name));
  }

  CAMLreturn(lst);
}
#else
value caml_curl_global_sslset(value v_ignored)
{
  const value *exception = caml_named_value("Curl.NotImplemented");
  if (NULL == exception) caml_invalid_argument("Curl.NotImplemented not registered");
  caml_raise_with_string(*exception, "curl_global_sslset");

  /* Not reached */
  return Val_unit;
}
value caml_curl_global_sslset_str(value v_ignored)
{
  /* Argument is ignored */
  return caml_curl_global_sslset(Val_unit);
}
value caml_curl_global_sslsetavail(value v_ignored)
{
  /* Argument is ignored */
  return caml_curl_global_sslset(Val_unit);
}
value caml_curl_global_sslsetavail_str(value v_ignored)
{
  /* Argument is ignored */
  return caml_curl_global_sslset(Val_unit);
}
#endif /* HAVE_DECL_CURL_GLOBAL_SSLSET */

/**
 **  curl_global_init helper function
 **/

value caml_curl_global_init(value initOption)
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
        caml_failwith("Invalid Initialization Option");
        break;
    }

    /* Keep compiler happy, we should never get here due to caml_failwith() */
    CAMLreturn(Val_unit);
}

/**
 **  curl_global_cleanup helper function
 **/

value caml_curl_global_cleanup(void)
{
    CAMLparam0();

    curl_global_cleanup();

    CAMLreturn(Val_unit);
}

/**
 ** curl_easy_init helper function
 **/
value caml_curl_easy_init(void)
{
    CAMLparam0();
    CAMLlocal1(result);

    result = caml_curl_alloc(newConnection());

    CAMLreturn(result);
}

value caml_curl_easy_reset(value conn)
{
    CAMLparam1(conn);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);
    curl_easy_reset(connection->handle);
    curl_easy_setopt(connection->handle, CURLOPT_PRIVATE, connection);
    resetOcamlValues(connection);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_setopt helper utility functions
 **/

#define SETOPT_FUNCTION_(name,suffix) \
static void handle_##name##suffix(Connection *conn, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLE_OK; \
    Store_field(conn->ocamlValues, Ocaml_##name##FUNCTION, option); \
    result = curl_easy_setopt(conn->handle, CURLOPT_##name##FUNCTION, cb_##name##suffix); \
    if (result != CURLE_OK) raiseError(conn, result); \
    result = curl_easy_setopt(conn->handle, CURLOPT_##name##DATA, conn); \
    if (result != CURLE_OK) raiseError(conn, result); \
    CAMLreturn0; \
}

#define SETOPT_FUNCTION(name) SETOPT_FUNCTION_(name,FUNCTION)
#define SETOPT_FUNCTION2(name) SETOPT_FUNCTION_(name,FUNCTION2)

SETOPT_FUNCTION( WRITE)
SETOPT_FUNCTION2( WRITE)
SETOPT_FUNCTION( READ)
SETOPT_FUNCTION2( READ)
SETOPT_FUNCTION( HEADER)
SETOPT_FUNCTION( PROGRESS)
#if HAVE_DECL_CURLOPT_XFERINFOFUNCTION
SETOPT_FUNCTION( XFERINFO)
#endif
#if HAVE_DECL_CURLOPT_PREREQFUNCTION
SETOPT_FUNCTION( PREREQ)
#endif
SETOPT_FUNCTION( DEBUG)
SETOPT_FUNCTION( SSH_KEY)

#if HAVE_DECL_CURLOPT_SEEKFUNCTION
SETOPT_FUNCTION( SEEK)
#endif

#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
SETOPT_FUNCTION( IOCTL)
#endif

SETOPT_FUNCTION( OPENSOCKET)
/* SETOPT_FUNCTION( CLOSESOCKET) */

static void handle_slist(Connection *conn, struct curl_slist** slist, CURLoption curl_option, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    free_curl_slist(*slist);
    *slist = NULL;

    while (Val_emptylist != option)
    {
        *slist = curl_slist_append(*slist, String_val(Field(option, 0)));

        option = Field(option, 1);
    }

    result = curl_easy_setopt(conn->handle, curl_option, *slist);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static long convert_bit_list(long *map, size_t map_size, value option)
{
    CAMLparam1(option);
    long bits = 0;
    int index;

    while (Val_emptylist != option)
    {
        index = Int_val(Field(option, 0));
        if ((index < 0) || ((size_t)index >= map_size))
          caml_invalid_argument("convert_bit_list");

        bits |= map[index];

        option = Field(option, 1);
    }

    CAMLreturnT(long, bits);
}

#define SETOPT_STRING(name) \
static void handle_##name(Connection *conn, value option) \
{ \
    CAMLparam1(option); \
    CURLcode result = CURLE_OK; \
\
    result = curl_easy_setopt(conn->handle, CURLOPT_##name, String_val(option)); \
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
    result = curl_easy_setopt(conn->handle, curl_option, conv_val(option)); \
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

#if HAVE_DECL_CURLOPT_MIMEPOST

static void handle_part_with_name(Connection* conn, curl_mimepart* part, value v_part_name_info)
{
  value v_data     = Field(v_part_name_info, 0);
  value v_name     = Field(v_part_name_info, 1);
  value v_filename = Field(v_part_name_info, 2);

  CURLcode result;
  value v_str = Field(v_data, 0);

  switch (Tag_val(v_data)) {
    case 0:
      result = curl_mime_data(part, String_val(v_str), caml_string_length(v_str));
      break;
    case 1:
      result = curl_mime_filedata(part, String_val(v_str));
      break;
    default:
      caml_failwith("Invalid MIMEPOST data value");
      break;
  }

  if (result != CURLE_OK) {
    raiseError(conn, result);
  }

  if (Is_block(v_name)) {
    result = curl_mime_name(part, String_val(Field(v_name, 0)));

    if (result != CURLE_OK) {
      raiseError(conn, result);
    }
  }

  if (Is_block(v_filename)) {
    result = curl_mime_filename(part, String_val(Field(v_filename, 0)));

    if (result != CURLE_OK) {
      raiseError(conn, result);
    }
  }
}

static void new_part(Connection* conn, curl_mime* mime, value v_part)
{
  value v_encoding = Field(v_part, 0);
  value v_headers  = Field(v_part, 1);
  value v_subparts = Field(v_part, 2);
  value v_data     = Field(v_part, 3);
  value v_str      = Field(v_data, 0);

  struct curl_slist *headers = NULL;
  CURLcode result;

  curl_mimepart *part = curl_mime_addpart(mime);

  switch (Int_val(v_encoding)) {
    case 0:
      result = curl_mime_encoder(part, "8bit");
      break;
    case 1:
      result = curl_mime_encoder(part, "binary");
      break;
    case 2:
      result = curl_mime_encoder(part, "7bit");
      break;
    case 3:
      result = curl_mime_encoder(part, "quoted-printable");
      break;
    case 4:
      result = curl_mime_encoder(part, "base64");
      break;
    case 5:
      result = CURLE_OK;
      break;
    default:
      caml_failwith("Invalid MIMEPOST encoding value");
      break;
  }

  if (result != CURLE_OK) {
    raiseError(conn, result);
  }

  while (v_headers != Val_emptylist) {
    headers = curl_slist_append(headers, String_val(Field(v_headers, 0)));
    v_headers = Field(v_headers, 1);
  }

  result = curl_mime_headers(part, headers, 1);

  if (result != CURLE_OK) {
    raiseError(conn, result);
  }

  switch (Tag_val(v_data)) {
    case 0:
      result = curl_mime_data(part, String_val(v_str), caml_string_length(v_str));
      break;
    case 1:
      result = curl_mime_filedata(part, String_val(v_str));
      break;
    case 2:
      handle_part_with_name(conn, part, v_data);
      break;
    default:
      caml_failwith("Invalid MIMEPOST data value");
      break;
  }

  if (result != CURLE_OK) {
    raiseError(conn, result);
  }

  if (v_subparts != Val_emptylist) {
    curl_mime *mime = curl_mime_init(conn->handle);

    while (v_subparts != Val_emptylist) {
      new_part(conn, mime, Field(v_subparts, 0));
      v_subparts = Field(v_subparts, 1);
    }

    result = curl_mime_subparts(part, mime);

    if (result != CURLE_OK) {
      raiseError(conn, result);
    }
  }
}

static void handle_MIMEPOST(Connection* conn, value v_subparts)
{
  CAMLparam1(v_subparts);
  curl_mime *mime = curl_mime_init(conn->handle);
  CURLcode result;

  curl_mime_free(conn->curl_MIMEPOST);
  conn->curl_MIMEPOST = mime;

  while (v_subparts != Val_emptylist) {
    new_part(conn, mime, Field(v_subparts, 0));
    v_subparts = Field(v_subparts, 1);
  }

  result = curl_easy_setopt(conn->handle, CURLOPT_MIMEPOST, mime);

  if (result != CURLE_OK) {
    raiseError(conn, result);
  }

  CAMLreturn0;
}

#endif

#define SETOPT_SLIST(name) \
static void handle_##name(Connection* conn, value option) \
{ \
  handle_slist(conn,&(conn->curl_##name),CURLOPT_##name,option); \
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
        caml_failwith("Invalid NETRC Option");
        break;
    }

    result = curl_easy_setopt(conn->handle,
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_ENCODING,
                                  "identity");
        break;

    case 1: /* CURL_ENCODING_DEFLATE */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_ENCODING,
                                  "deflate");
        break;

    case 2: /* CURL_ENCODING_GZIP */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_ENCODING,
                                  "gzip");
        break;

    case 3: /* CURL_ENCODING_ANY */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_ENCODING,
                                  "");
        break;

    default:
        caml_failwith("Invalid Encoding Option");
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

    conn->curl_ERRORBUFFER = (char*)malloc(sizeof(char) * CURL_ERROR_SIZE);

    result = curl_easy_setopt(conn->handle,
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

    if (conn->curl_POSTFIELDS != NULL)
        free(conn->curl_POSTFIELDS);

    conn->curl_POSTFIELDS = strdup_ml(option);

    result = curl_easy_setopt(conn->handle,
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

long sslOptionMap[] = {
#ifdef CURLSSLOPT_ALLOW_BEAST
  CURLSSLOPT_ALLOW_BEAST,
#else
  0,
#endif
#ifdef CURLSSLOPT_NO_REVOKE
  CURLSSLOPT_NO_REVOKE,
#else
  0,
#endif
#ifdef CURLSSLOPT_NO_PARTIALCHAIN
  CURLSSLOPT_NO_PARTIALCHAIN,
#else
  0,
#endif
#ifdef CURLSSLOPT_REVOKE_BEST_EFFORT
  CURLSSLOPT_REVOKE_BEST_EFFORT,
#else
  0,
#endif
#ifdef CURLSSLOPT_NATIVE_CA
  CURLSSLOPT_NATIVE_CA,
#else
  0,
#endif
#ifdef CURLSSLOPT_AUTO_CLIENT_CERT
  CURLSSLOPT_AUTO_CLIENT_CERT,
#else
  0,
#endif
};

#if HAVE_DECL_CURLOPT_SSL_OPTIONS
static void handle_SSL_OPTIONS(Connection *conn, value opts)
{
  CAMLparam1(opts);
  CURLcode result = CURLE_OK;
  long bits = convert_bit_list(sslOptionMap, sizeof(sslOptionMap) / sizeof(sslOptionMap[0]), opts);

  result = curl_easy_setopt(conn->handle, CURLOPT_SSL_OPTIONS, bits);

  if (result != CURLE_OK)
    raiseError(conn, result);

  CAMLreturn0;
}
#endif

static void handle_HTTPPOST(Connection *conn, value option)
{
    CAMLparam1(option);
    CAMLlocal3(listIter, formItem, contentType);
    CURLcode result = CURLE_OK;

    listIter = option;

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
                caml_failwith("Incorrect CURLFORM_CONTENT parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_COPYCONTENTS,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTSLENGTH,
                             caml_string_length(Field(formItem, 1)),
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
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_COPYCONTENTS,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTSLENGTH,
                             caml_string_length(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
                             CURLFORM_END);
            }
            else
            {
                caml_failwith("Incorrect CURLFORM_CONTENT parameters");
            }
            break;

        case 1: /* CURLFORM_FILECONTENT */
            if (Wosize_val(formItem) < 3)
            {
                caml_failwith("Incorrect CURLFORM_FILECONTENT parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             caml_string_length(Field(formItem, 0)),
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
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_FILECONTENT,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
                             CURLFORM_END);
            }
            else
            {
                caml_failwith("Incorrect CURLFORM_FILECONTENT parameters");
            }
            break;

        case 2: /* CURLFORM_FILE */
            if (Wosize_val(formItem) < 3)
            {
                caml_failwith("Incorrect CURLFORM_FILE parameters");
            }

            if (Is_long(Field(formItem, 2)) &&
                Long_val(Field(formItem, 2)) == 0)
            {
                curl_formadd(&conn->httpPostFirst,
                             &conn->httpPostLast,
                             CURLFORM_COPYNAME,
                             String_val(Field(formItem, 0)),
                             CURLFORM_NAMELENGTH,
                             caml_string_length(Field(formItem, 0)),
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
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_FILE,
                             String_val(Field(formItem, 1)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
                             CURLFORM_END);
            }
            else
            {
                caml_failwith("Incorrect CURLFORM_FILE parameters");
            }
            break;

        case 3: /* CURLFORM_BUFFER */
            if (Wosize_val(formItem) < 4)
            {
                caml_failwith("Incorrect CURLFORM_BUFFER parameters");
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
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             String_val(Field(formItem, 1)),
                             CURLFORM_BUFFERPTR,
                             conn->httpPostBuffers->data,
                             CURLFORM_BUFFERLENGTH,
                             caml_string_length(Field(formItem, 2)),
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
                             caml_string_length(Field(formItem, 0)),
                             CURLFORM_BUFFER,
                             String_val(Field(formItem, 1)),
                             CURLFORM_BUFFERPTR,
                             conn->httpPostBuffers->data,
                             CURLFORM_BUFFERLENGTH,
                             caml_string_length(Field(formItem, 2)),
                             CURLFORM_CONTENTTYPE,
                             String_val(Field(contentType, 0)),
                             CURLFORM_END);
            }
            else
            {
                caml_failwith("Incorrect CURLFORM_BUFFER parameters");
            }
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->handle,
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
#if HAVE_DECL_CURLOPT_CERTINFO
SETOPT_BOOL( CERTINFO)
#endif

#if !HAVE_DECL_CURL_SSLVERSION_TLSV1_0
#define CURL_SSLVERSION_TLSv1_0 CURL_SSLVERSION_TLSv1
#endif

#if !HAVE_DECL_CURL_SSLVERSION_TLSV1_1
#define CURL_SSLVERSION_TLSv1_1 CURL_SSLVERSION_TLSv1
#endif

#if !HAVE_DECL_CURL_SSLVERSION_TLSV1_2
#define CURL_SSLVERSION_TLSv1_2 CURL_SSLVERSION_TLSv1
#endif

#if !HAVE_DECL_CURL_SSLVERSION_TLSV1_3
#define CURL_SSLVERSION_TLSv1_3 CURL_SSLVERSION_TLSv1
#endif

static void handle_SSLVERSION(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    int v = CURL_SSLVERSION_DEFAULT;

    switch (Long_val(option))
    {
    case 0: v = CURL_SSLVERSION_DEFAULT; break;
    case 1: v = CURL_SSLVERSION_TLSv1; break;
    case 2: v = CURL_SSLVERSION_SSLv2; break;
    case 3: v = CURL_SSLVERSION_SSLv3; break;
    case 4: v = CURL_SSLVERSION_TLSv1_0; break;
    case 5: v = CURL_SSLVERSION_TLSv1_1; break;
    case 6: v = CURL_SSLVERSION_TLSv1_2; break;
    case 7: v = CURL_SSLVERSION_TLSv1_3; break;
    default:
        caml_failwith("Invalid SSLVERSION Option");
        break;
    }

    result = curl_easy_setopt(conn->handle, CURLOPT_SSLVERSION, v);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

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
        caml_failwith("Invalid TIMECOND Option");
        break;
    }

    result = curl_easy_setopt(conn->handle, CURLOPT_TIMECONDITION, timecond);

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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_KRB4LEVEL,
                                  NULL);
        break;

    case 1: /* KRB4_CLEAR */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_KRB4LEVEL,
                                  "clear");
        break;

    case 2: /* KRB4_SAFE */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_KRB4LEVEL,
                                  "safe");
        break;

    case 3: /* KRB4_CONFIDENTIAL */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_KRB4LEVEL,
                                  "confidential");
        break;

    case 4: /* KRB4_PRIVATE */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_KRB4LEVEL,
                                  "private");
        break;

    default:
        caml_failwith("Invalid KRB4 Option");
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_CLOSEPOLICY,
                                  CURLCLOSEPOLICY_OLDEST);
        break;

    case 1: /* CLOSEPOLICY_LEAST_RECENTLY_USED */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_CLOSEPOLICY,
                                  CURLCLOSEPOLICY_LEAST_RECENTLY_USED);
        break;

    default:
        caml_failwith("Invalid CLOSEPOLICY Option");
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_SSL_VERIFYHOST,
                                  /* map EXISTENCE to HOSTNAME */
                                  Long_val(option) == 0 ? 0 : 2);
        break;

    default:
        caml_failwith("Invalid SSLVERIFYHOST Option");
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

    long version = CURL_HTTP_VERSION_NONE;

    switch (Long_val(option))
    {
    case 0: version = CURL_HTTP_VERSION_NONE; break;
    case 1: version = CURL_HTTP_VERSION_1_0; break;
    case 2: version = CURL_HTTP_VERSION_1_1; break;
    case 3:
#if HAVE_DECL_CURL_HTTP_VERSION_2
      version = CURL_HTTP_VERSION_2;
#elif HAVE_DECL_CURL_HTTP_VERSION_2_0
      version = CURL_HTTP_VERSION_2_0;
#endif
      break;
    case 4:
#if HAVE_DECL_CURL_HTTP_VERSION_2TLS
      version = CURL_HTTP_VERSION_2TLS;
#endif
      break;
    case 5:
#if HAVE_DECL_CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE
      version = CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE;
#endif
      break;
    case 6:
#if HAVE_DECL_CURL_HTTP_VERSION_3
      version = CURL_HTTP_VERSION_3;
#endif
      break;
    /* sync check_enums */
    default:
      caml_invalid_argument("CURLOPT_HTTP_VERSION");
      break;
    }

    result = curl_easy_setopt(conn->handle, CURLOPT_HTTP_VERSION, version);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}

static long ocaml_HTTP_VERSION(long curl_version)
{
    switch (curl_version)
    {
    case CURL_HTTP_VERSION_NONE: return 0;
    case CURL_HTTP_VERSION_1_0: return 1;
    case CURL_HTTP_VERSION_1_1: return 2;
#if HAVE_DECL_CURL_HTTP_VERSION_2
    case CURL_HTTP_VERSION_2: return 3;
#elif HAVE_DECL_CURL_HTTP_VERSION_2_0
    case CURL_HTTP_VERSION_2_0: return 3;
#endif
#if HAVE_DECL_CURL_HTTP_VERSION_2TLS
    case CURL_HTTP_VERSION_2TLS: return 4;
#endif
#if HAVE_DECL_CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE
    case CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE: return 5;
#endif
#if HAVE_DECL_CURL_HTTP_VERSION_3
    case CURL_HTTP_VERSION_3: return 6;
#endif
    /* sync check_enums */
    default: return 0;
    }
}

SETOPT_BOOL( FTP_USE_EPSV)
SETOPT_LONG( DNS_CACHE_TIMEOUT)
SETOPT_BOOL( DNS_USE_GLOBAL_CACHE)

static void handle_PRIVATE(Connection *conn, value option)
{
    CAMLparam1(option);
    Store_field(conn->ocamlValues, Ocaml_PRIVATE, option);
    CAMLreturn0;
}

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
            caml_failwith("Invalid HTTPAUTH Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->handle,
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
            caml_failwith("Invalid HTTPAUTH Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->handle,
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_WHATEVER);
        break;

    case 1: /* CURL_IPRESOLVE_V4 */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_V4);
        break;

    case 2: /* CURL_IPRESOLVE_V6 */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_IPRESOLVE,
                                  CURL_IPRESOLVE_V6);
        break;

    default:
        caml_failwith("Invalid IPRESOLVE Value");
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_NONE);
        break;

    case 1: /* CURLFTPSSL_TRY */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_TRY);
        break;

    case 2: /* CURLFTPSSL_CONTROL */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_CONTROL);
        break;

    case 3: /* CURLFTPSSL_ALL */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL,
                                  CURLFTPSSL_ALL);
        break;

    default:
        caml_failwith("Invalid FTP_SSL Value");
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

#if HAVE_DECL_CURLOPT_TCP_FASTOPEN
SETOPT_BOOL( TCP_FASTOPEN)
#endif

#if HAVE_DECL_CURLOPT_FTPSSLAUTH
static void handle_FTPSSLAUTH(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;

    switch (Long_val(option))
    {
    case 0: /* CURLFTPAUTH_DEFAULT */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_DEFAULT);
        break;

    case 1: /* CURLFTPAUTH_SSL */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_SSL);
        break;

    case 2: /* CURLFTPAUTH_TLS */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTPSSLAUTH,
                                  CURLFTPAUTH_TLS);
        break;

    default:
        caml_failwith("Invalid FTPSSLAUTH value");
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_DEFAULT);
        break;

    case 1: /* CURLFTMETHOD_MULTICWD */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_MULTICWD);
        break;

    case 2: /* CURLFTPMETHOD_NOCWD */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_NOCWD);
        break;

    case 3: /* CURLFTPMETHOD_SINGLECWD */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_FILEMETHOD,
                                  CURLFTPMETHOD_SINGLECWD);

    default:
        caml_failwith("Invalid FTP_FILEMETHOD value");
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
            caml_failwith("Invalid CURLSSH_AUTH_TYPES Value");
            break;
        }

        listIter = Field(listIter, 1);
    }

    result = curl_easy_setopt(conn->handle,
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
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_NONE);
        break;

    case 1: /* CURLFTPSSL_CCC_PASSIVE */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_PASSIVE);
        break;

    case 2: /* CURLFTPSSL_CCC_ACTIVE */
        result = curl_easy_setopt(conn->handle,
                                  CURLOPT_FTP_SSL_CCC,
                                  CURLFTPSSL_CCC_ACTIVE);
        break;

    default:
        caml_failwith("Invalid FTPSSL_CCC value");
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
        caml_failwith("Invalid curl proxy type");
    }

    result = curl_easy_setopt(conn->handle,
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
    long bits = convert_bit_list(protoMap, sizeof(protoMap) / sizeof(protoMap[0]), option);

    result = curl_easy_setopt(conn->handle, curlopt, bits);

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

#if HAVE_DECL_CURLOPT_PIPEWAIT
SETOPT_BOOL( PIPEWAIT)
#endif

#if HAVE_DECL_CURLOPT_USERNAME
SETOPT_STRING( USERNAME)
#endif

#if HAVE_DECL_CURLOPT_PASSWORD
SETOPT_STRING( PASSWORD)
#endif

#if HAVE_DECL_CURLOPT_LOGIN_OPTIONS
SETOPT_STRING( LOGIN_OPTIONS)
#endif

#if HAVE_DECL_CURLOPT_CONNECT_TO
SETOPT_SLIST( CONNECT_TO)
#endif

#if HAVE_DECL_CURLOPT_POSTREDIR

static int curlPostRedir_table[] = {
 CURL_REDIR_POST_ALL,
#if defined(CURL_REDIR_POST_301)
  CURL_REDIR_POST_301,
#else
  0,
#endif
#if defined(CURL_REDIR_POST_302)
  CURL_REDIR_POST_302,
#else
  0,
#endif
#if defined(CURL_REDIR_POST_303)
  CURL_REDIR_POST_303,
#else
  0,
#endif
};

static void handle_POSTREDIR(Connection *conn, value option)
{
    CAMLparam1(option);
    CURLcode result = CURLE_OK;
    long bitmask = caml_convert_flag_list(option, curlPostRedir_table);

    result = curl_easy_setopt(conn->handle,
                              CURLOPT_POSTREDIR,
                              bitmask);

    if (result != CURLE_OK)
        raiseError(conn, result);

    CAMLreturn0;
}
#endif

SETOPT_VAL( SSH_KNOWNHOSTS, String_val)

SETOPT_LONG( BUFFERSIZE)

#if HAVE_DECL_CURLOPT_DOH_URL
  SETOPT_STRING( DOH_URL)
#endif

#if HAVE_DECL_CURLOPT_AWS_SIGV4
  SETOPT_STRING( AWS_SIGV4)
#endif

SETOPT_BOOL( TCP_KEEPALIVE)

SETOPT_LONG( TCP_KEEPIDLE)

SETOPT_LONG( TCP_KEEPINTVL)

/**
 **  curl_easy_setopt helper function
 **/

#define HAVE(name) { handle_ ## name, "CURLOPT_"#name }
#define HAVENOT(name) { NULL, "CURLOPT_"#name }

CURLOptionMapping implementedOptionMap[] =
{
  HAVE(WRITEFUNCTION),
  HAVE(READFUNCTION),
  HAVE(INFILESIZE),
  HAVE(URL),
  HAVE(PROXY),
  HAVE(PROXYPORT),
  HAVE(HTTPPROXYTUNNEL),
  HAVE(VERBOSE),
  HAVE(HEADER),
  HAVE(NOPROGRESS),
#if HAVE_DECL_CURLOPT_NOSIGNAL
  HAVE(NOSIGNAL),
#else
  HAVENOT(NOSIGNAL),
#endif
  HAVE(NOBODY),
  HAVE(FAILONERROR),
  HAVE(UPLOAD),
  HAVE(POST),
  HAVE(FTPLISTONLY),
  HAVE(FTPAPPEND),
  HAVE(NETRC),
#if HAVE_DECL_CURLOPT_ENCODING
  HAVE(ENCODING),
#else
  HAVENOT(ENCODING),
#endif
  HAVE(FOLLOWLOCATION),
  HAVE(TRANSFERTEXT),
  HAVE(PUT),
  HAVE(USERPWD),
  HAVE(PROXYUSERPWD),
  HAVE(RANGE),
  HAVE(ERRORBUFFER), /* mutable buffer, as output value, do not duplicate */
  HAVE(TIMEOUT),
  HAVE(POSTFIELDS),
  HAVE(POSTFIELDSIZE),
  HAVE(REFERER),
  HAVE(USERAGENT),
  HAVE(FTPPORT),
  HAVE(LOW_SPEED_LIMIT),
  HAVE(LOW_SPEED_TIME),
  HAVE(RESUME_FROM),
  HAVE(COOKIE),
  HAVE(HTTPHEADER),
  HAVE(HTTPPOST),
  HAVE(SSLCERT),
  HAVE(SSLCERTTYPE),
  HAVE(SSLCERTPASSWD),
  HAVE(SSLKEY),
  HAVE(SSLKEYTYPE),
  HAVE(SSLKEYPASSWD),
  HAVE(SSLENGINE),
  HAVE(SSLENGINE_DEFAULT),
  HAVE(CRLF),
  HAVE(QUOTE),
  HAVE(POSTQUOTE),
  HAVE(HEADERFUNCTION),
  HAVE(COOKIEFILE),
  HAVE(SSLVERSION),
  HAVE(TIMECONDITION),
  HAVE(TIMEVALUE),
  HAVE(CUSTOMREQUEST),
  HAVE(INTERFACE),
  HAVE(KRB4LEVEL),
  HAVE(PROGRESSFUNCTION),
  HAVE(SSL_VERIFYPEER),
  HAVE(CAINFO),
  HAVE(CAPATH),
  HAVE(FILETIME),
  HAVE(MAXREDIRS),
  HAVE(MAXCONNECTS),
  HAVE(CLOSEPOLICY),
  HAVE(FRESH_CONNECT),
  HAVE(FORBID_REUSE),
  HAVE(RANDOM_FILE),
  HAVE(EGDSOCKET),
  HAVE(CONNECTTIMEOUT),
  HAVE(HTTPGET),
  HAVE(SSL_VERIFYHOST),
  HAVE(COOKIEJAR),
  HAVE(SSL_CIPHER_LIST),
  HAVE(HTTP_VERSION),
  HAVE(FTP_USE_EPSV),
  HAVE(DNS_CACHE_TIMEOUT),
  HAVE(DNS_USE_GLOBAL_CACHE),
  HAVE(DEBUGFUNCTION),
  HAVE(PRIVATE),
#if HAVE_DECL_CURLOPT_HTTP200ALIASES
  HAVE(HTTP200ALIASES),
#else
  HAVENOT(HTTP200ALIASES),
#endif
#if HAVE_DECL_CURLOPT_UNRESTRICTED_AUTH
  HAVE(UNRESTRICTED_AUTH),
#else
  HAVENOT(UNRESTRICTED_AUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_USE_EPRT
  HAVE(FTP_USE_EPRT),
#else
  HAVENOT(FTP_USE_EPRT),
#endif
#if HAVE_DECL_CURLOPT_HTTPAUTH
  HAVE(HTTPAUTH),
#else
  HAVENOT(HTTPAUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_CREATE_MISSING_DIRS
  HAVE(FTP_CREATE_MISSING_DIRS),
#else
  HAVENOT(FTP_CREATE_MISSING_DIRS),
#endif
#if HAVE_DECL_CURLOPT_PROXYAUTH
  HAVE(PROXYAUTH),
#else
  HAVENOT(PROXYAUTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_RESPONSE_TIMEOUT
  HAVE(FTP_RESPONSE_TIMEOUT),
#else
  HAVENOT(FTP_RESPONSE_TIMEOUT),
#endif
#if HAVE_DECL_CURLOPT_IPRESOLVE
  HAVE(IPRESOLVE),
#else
  HAVENOT(IPRESOLVE),
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE
  HAVE(MAXFILESIZE),
#else
  HAVENOT(MAXFILESIZE),
#endif
#if HAVE_DECL_CURLOPT_INFILESIZE_LARGE
  HAVE(INFILESIZE_LARGE),
#else
  HAVENOT(INFILESIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_RESUME_FROM_LARGE
  HAVE(RESUME_FROM_LARGE),
#else
  HAVENOT(RESUME_FROM_LARGE),
#endif
#if HAVE_DECL_CURLOPT_MAXFILESIZE_LARGE
  HAVE(MAXFILESIZE_LARGE),
#else
  HAVENOT(MAXFILESIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_NETRC_FILE
  HAVE(NETRC_FILE),
#else
  HAVENOT(NETRC_FILE),
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL
  HAVE(FTP_SSL),
#else
  HAVENOT(FTP_SSL),
#endif
#if HAVE_DECL_CURLOPT_POSTFIELDSIZE_LARGE
  HAVE(POSTFIELDSIZE_LARGE),
#else
  HAVENOT(POSTFIELDSIZE_LARGE),
#endif
#if HAVE_DECL_CURLOPT_TCP_NODELAY
  HAVE(TCP_NODELAY),
#else
  HAVENOT(TCP_NODELAY),
#endif
#if HAVE_DECL_CURLOPT_TCP_FASTOPEN
  HAVE(TCP_FASTOPEN),
#else
  HAVENOT(TCP_FASTOPEN),
#endif
#if HAVE_DECL_CURLOPT_FTPSSLAUTH
  HAVE(FTPSSLAUTH),
#else
  HAVENOT(FTPSSLAUTH),
#endif
#if HAVE_DECL_CURLOPT_IOCTLFUNCTION
  HAVE(IOCTLFUNCTION),
#else
  HAVENOT(IOCTLFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_FTP_ACCOUNT
  HAVE(FTP_ACCOUNT),
#else
  HAVENOT(FTP_ACCOUNT),
#endif
#if HAVE_DECL_CURLOPT_COOKIELIST
  HAVE(COOKIELIST),
#else
  HAVENOT(COOKIELIST),
#endif
#if HAVE_DECL_CURLOPT_IGNORE_CONTENT_LENGTH
  HAVE(IGNORE_CONTENT_LENGTH),
#else
  HAVENOT(IGNORE_CONTENT_LENGTH),
#endif
#if HAVE_DECL_CURLOPT_FTP_SKIP_PASV_IP
  HAVE(FTP_SKIP_PASV_IP),
#else
  HAVENOT(FTP_SKIP_PASV_IP),
#endif
#if HAVE_DECL_CURLOPT_FTP_FILEMETHOD
  HAVE(FTP_FILEMETHOD),
#else
  HAVENOT(FTP_FILEMETHOD),
#endif
#if HAVE_DECL_CURLOPT_LOCALPORT
  HAVE(LOCALPORT),
#else
  HAVENOT(LOCALPORT),
#endif
#if HAVE_DECL_CURLOPT_LOCALPORTRANGE
  HAVE(LOCALPORTRANGE),
#else
  HAVENOT(LOCALPORTRANGE),
#endif
#if HAVE_DECL_CURLOPT_CONNECT_ONLY
  HAVE(CONNECT_ONLY),
#else
  HAVENOT(CONNECT_ONLY),
#endif
#if HAVE_DECL_CURLOPT_MAX_SEND_SPEED_LARGE
  HAVE(MAX_SEND_SPEED_LARGE),
#else
  HAVENOT(MAX_SEND_SPEED_LARGE),
#endif
#if HAVE_DECL_CURLOPT_MAX_RECV_SPEED_LARGE
  HAVE(MAX_RECV_SPEED_LARGE),
#else
  HAVENOT(MAX_RECV_SPEED_LARGE),
#endif
#if HAVE_DECL_CURLOPT_FTP_ALTERNATIVE_TO_USER
  HAVE(FTP_ALTERNATIVE_TO_USER),
#else
  HAVENOT(FTP_ALTERNATIVE_TO_USER),
#endif
#if HAVE_DECL_CURLOPT_SSL_SESSIONID_CACHE
  HAVE(SSL_SESSIONID_CACHE),
#else
  HAVENOT(SSL_SESSIONID_CACHE),
#endif
#if HAVE_DECL_CURLOPT_SSH_AUTH_TYPES
  HAVE(SSH_AUTH_TYPES),
#else
  HAVENOT(SSH_AUTH_TYPES),
#endif
#if HAVE_DECL_CURLOPT_SSH_PUBLIC_KEYFILE
  HAVE(SSH_PUBLIC_KEYFILE),
#else
  HAVENOT(SSH_PUBLIC_KEYFILE),
#endif
#if HAVE_DECL_CURLOPT_SSH_PRIVATE_KEYFILE
  HAVE(SSH_PRIVATE_KEYFILE),
#else
  HAVENOT(SSH_PRIVATE_KEYFILE),
#endif
#if HAVE_DECL_CURLOPT_FTP_SSL_CCC
  HAVE(FTP_SSL_CCC),
#else
  HAVENOT(FTP_SSL_CCC),
#endif
#if HAVE_DECL_CURLOPT_TIMEOUT_MS
  HAVE(TIMEOUT_MS),
#else
  HAVENOT(TIMEOUT_MS),
#endif
#if HAVE_DECL_CURLOPT_CONNECTTIMEOUT_MS
  HAVE(CONNECTTIMEOUT_MS),
#else
  HAVENOT(CONNECTTIMEOUT_MS),
#endif
#if HAVE_DECL_CURLOPT_HTTP_TRANSFER_DECODING
  HAVE(HTTP_TRANSFER_DECODING),
#else
  HAVENOT(HTTP_TRANSFER_DECODING),
#endif
#if HAVE_DECL_CURLOPT_HTTP_CONTENT_DECODING
  HAVE(HTTP_CONTENT_DECODING),
#else
  HAVENOT(HTTP_CONTENT_DECODING),
#endif
#if HAVE_DECL_CURLOPT_NEW_FILE_PERMS
  HAVE(NEW_FILE_PERMS),
#else
  HAVENOT(NEW_FILE_PERMS),
#endif
#if HAVE_DECL_CURLOPT_NEW_DIRECTORY_PERMS
  HAVE(NEW_DIRECTORY_PERMS),
#else
  HAVENOT(NEW_DIRECTORY_PERMS),
#endif
#if HAVE_DECL_CURLOPT_POST301
  HAVE(POST301),
#else
  HAVENOT(POST301),
#endif
#if HAVE_DECL_CURLOPT_SSH_HOST_PUBLIC_KEY_MD5
  HAVE(SSH_HOST_PUBLIC_KEY_MD5),
#else
  HAVENOT(SSH_HOST_PUBLIC_KEY_MD5),
#endif
#if HAVE_DECL_CURLOPT_COPYPOSTFIELDS
  HAVE(COPYPOSTFIELDS),
#else
  HAVENOT(COPYPOSTFIELDS),
#endif
#if HAVE_DECL_CURLOPT_PROXY_TRANSFER_MODE
  HAVE(PROXY_TRANSFER_MODE),
#else
  HAVENOT(PROXY_TRANSFER_MODE),
#endif
#if HAVE_DECL_CURLOPT_SEEKFUNCTION
  HAVE(SEEKFUNCTION),
#else
  HAVENOT(SEEKFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_AUTOREFERER
  HAVE(AUTOREFERER),
#else
  HAVENOT(AUTOREFERER),
#endif
  HAVE(OPENSOCKETFUNCTION),
  /*HAVE(CLOSESOCKETFUNCTION),*/
#if HAVE_DECL_CURLOPT_PROXYTYPE
  HAVE(PROXYTYPE),
#else
  HAVENOT(PROXYTYPE),
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS
  HAVE(PROTOCOLS),
#else
  HAVENOT(PROTOCOLS),
#endif
#if HAVE_DECL_CURLOPT_REDIR_PROTOCOLS
  HAVE(REDIR_PROTOCOLS),
#else
  HAVENOT(REDIR_PROTOCOLS),
#endif
#if HAVE_DECL_CURLOPT_RESOLVE
  HAVE(RESOLVE),
#else
  HAVENOT(RESOLVE),
#endif
#if HAVE_DECL_CURLOPT_DNS_SERVERS
  HAVE(DNS_SERVERS),
#else
  HAVENOT(DNS_SERVERS),
#endif
#if HAVE_DECL_CURLOPT_MAIL_FROM
  HAVE(MAIL_FROM),
#else
  HAVENOT(MAIL_FROM),
#endif
#if HAVE_DECL_CURLOPT_MAIL_RCPT
  HAVE(MAIL_RCPT),
#else
  HAVENOT(MAIL_RCPT),
#endif
#if HAVE_DECL_CURLOPT_PIPEWAIT
  HAVE(PIPEWAIT),
#else
  HAVENOT(PIPEWAIT),
#endif
#if HAVE_DECL_CURLOPT_CERTINFO
  HAVE(CERTINFO),
#else
  HAVENOT(CERTINFO),
#endif
#if HAVE_DECL_CURLOPT_USERNAME
  HAVE(USERNAME),
#else
  HAVENOT(USERNAME),
#endif
#if HAVE_DECL_CURLOPT_PASSWORD
  HAVE(PASSWORD),
#else
  HAVENOT(PASSWORD),
#endif
#if HAVE_DECL_CURLOPT_LOGIN_OPTIONS
  HAVE(LOGIN_OPTIONS),
#else
  HAVENOT(LOGIN_OPTIONS),
#endif
#if HAVE_DECL_CURLOPT_CONNECT_TO
  HAVE(CONNECT_TO),
#else
  HAVENOT(CONNECT_TO),
#endif
#if HAVE_DECL_CURLOPT_POSTREDIR
  HAVE(POSTREDIR),
#else
  HAVENOT(POSTREDIR),
#endif
#if HAVE_DECL_CURLOPT_MIMEPOST
  HAVE(MIMEPOST),
#else
  HAVENOT(MIMEPOST),
#endif
  HAVE(SSH_KNOWNHOSTS),
  HAVE(SSH_KEYFUNCTION),
  HAVE(BUFFERSIZE),
#if HAVE_DECL_CURLOPT_DOH_URL
  HAVE(DOH_URL),
#else
  HAVENOT(DOH_URL),
#endif
#if HAVE_DECL_CURLOPT_SSL_OPTIONS
  HAVE(SSL_OPTIONS),
#else
  HAVENOT(SSL_OPTIONS),
#endif
  HAVE(WRITEFUNCTION2),
  HAVE(READFUNCTION2),
#if HAVE_DECL_CURLOPT_XFERINFOFUNCTION
  HAVE(XFERINFOFUNCTION),
#else
  HAVENOT(XFERINFOFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_PREREQFUNCTION
  HAVE(PREREQFUNCTION),
#else
  HAVENOT(PREREQFUNCTION),
#endif
#if HAVE_DECL_CURLOPT_AWS_SIGV4
  HAVE(AWS_SIGV4),
#else
  HAVENOT(AWS_SIGV4),
#endif
HAVE(TCP_KEEPALIVE),
HAVE(TCP_KEEPIDLE),
HAVE(TCP_KEEPINTVL),
};

value caml_curl_easy_setopt(value conn, value option)
{
    CAMLparam2(conn, option);
    CAMLlocal1(data);
    Connection *connection = Connection_val(conn);
    CURLOptionMapping* thisOption = NULL;
    static const value* exception = NULL;

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

value caml_curl_easy_perform(value conn)
{
    CAMLparam1(conn);
    CURLcode result = CURLE_OK;
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    caml_enter_blocking_section();
    result = curl_easy_perform(connection->handle);
    caml_leave_blocking_section();

    if (result != CURLE_OK)
        raiseError(connection, result);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_cleanup helper function
 **/

value caml_curl_easy_cleanup(value conn)
{
    CAMLparam1(conn);
    Connection *connection = Connection_val(conn);

    checkConnection(connection);

    removeConnection(connection, 0);

    CAMLreturn(Val_unit);
}

/**
 **  curl_easy_getinfo helper function
 **/

enum GetInfoResultType {
    StringValue, LongValue, DoubleValue, StringListValue, StringListListValue,
    SocketValue, OCamlValue, /* keep last - no matching OCaml CURLINFO_ constructor */
};

value convertStringList(struct curl_slist *p)
{
    CAMLparam0();
    CAMLlocal3(result, current, next);

    result = Val_emptylist;
    current = Val_emptylist;
    next = Val_emptylist;

    while (p != NULL)
    {
        next = caml_alloc_tuple(2);
        Store_field(next, 0, caml_copy_string(p->data));
        Store_field(next, 1, Val_emptylist);

        if (result == Val_emptylist)
            result = next;

        if (current != Val_emptylist)
          Store_field(current, 1, next);

        current = next;

        p = p->next;
    }

    CAMLreturn(result);
}

value caml_curl_easy_getinfo(value conn, value option)
{
    CAMLparam2(conn, option);
    CAMLlocal3(result, current, next);
    CURLcode curlResult;
    Connection *connection = Connection_val(conn);
    enum GetInfoResultType resultType;
    char *strValue = NULL;
    double doubleValue;
    long longValue;
    curl_socket_t socketValue;
    struct curl_slist *stringListValue = NULL;
#if HAVE_DECL_CURLINFO_CERTINFO
    int i;
    union {
      struct curl_slist    *to_info;
      struct curl_certinfo *to_certinfo;
    } ptr;
#endif

    checkConnection(connection);

    switch(Long_val(option))
    {
#if HAVE_DECL_CURLINFO_EFFECTIVE_URL
    case 0: /* CURLINFO_EFFECTIVE_URL */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
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

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_RESPONSE_CODE,
                                       &longValue);
#else
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_HTTP_CODE,
                                       &longValue);
#endif
        break;
#endif

#if HAVE_DECL_CURLINFO_TOTAL_TIME
    case 3: /* CURLINFO_TOTAL_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_TOTAL_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_NAMELOOKUP_TIME
    case 4: /* CURLINFO_NAMELOOKUP_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_NAMELOOKUP_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONNECT_TIME
    case 5: /* CURLINFO_CONNECT_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CONNECT_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_PRETRANSFER_TIME
    case 6: /* CURLINFO_PRETRANSFER_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_PRETRANSFER_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SIZE_UPLOAD
    case 7: /* CURLINFO_SIZE_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SIZE_UPLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SIZE_DOWNLOAD
    case 8: /* CURLINFO_SIZE_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SIZE_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SPEED_DOWNLOAD
    case 9: /* CURLINFO_SPEED_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SPEED_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SPEED_UPLOAD
    case 10: /* CURLINFO_SPEED_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SPEED_UPLOAD,
                                       &doubleValue);
        break;

#endif

#if HAVE_DECL_CURLINFO_HEADER_SIZE
    case 11: /* CURLINFO_HEADER_SIZE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_HEADER_SIZE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REQUEST_SIZE
    case 12: /* CURLINFO_REQUEST_SIZE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_REQUEST_SIZE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SSL_VERIFYRESULT
    case 13: /* CURLINFO_SSL_VERIFYRESULT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SSL_VERIFYRESULT,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_FILETIME
    case 14: /* CURLINFO_FILETIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_FILETIME,
                                       &longValue);

        doubleValue = longValue;
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_LENGTH_DOWNLOAD
    case 15: /* CURLINFO_CONTENT_LENGTH_DOWNLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_LENGTH_UPLOAD
    case 16: /* CURLINFO_CONTENT_LENGTH_UPLOAD */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CONTENT_LENGTH_UPLOAD,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_STARTTRANSFER_TIME
    case 17: /* CURLINFO_STARTTRANSFER_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_STARTTRANSFER_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_CONTENT_TYPE
    case 18: /* CURLINFO_CONTENT_TYPE */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CONTENT_TYPE,
                                       &strValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_TIME
    case 19: /* CURLINFO_REDIRECT_TIME */
        resultType = DoubleValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_REDIRECT_TIME,
                                       &doubleValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_COUNT
    case 20: /* CURLINFO_REDIRECT_COUNT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_REDIRECT_COUNT,
                                       &longValue);
        break;
#endif

    case 21: /* CURLINFO_PRIVATE */
        resultType = OCamlValue;
        curlResult = CURLE_OK;
        result = caml_alloc(1, StringValue);
        Store_field(result, 0, Field(connection->ocamlValues, Ocaml_PRIVATE));
        break;

#if HAVE_DECL_CURLINFO_HTTP_CONNECTCODE
    case 22: /* CURLINFO_HTTP_CONNECTCODE */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_HTTP_CONNECTCODE,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_HTTPAUTH_AVAIL
    case 23: /* CURLINFO_HTTPAUTH_AVAIL */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_HTTPAUTH_AVAIL,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_PROXYAUTH_AVAIL
    case 24: /* CURLINFO_PROXYAUTH_AVAIL */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_PROXYAUTH_AVAIL,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_OS_ERRNO
    case 25: /* CURLINFO_OS_ERRNO */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_OS_ERRNO,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_NUM_CONNECTS
    case 26: /* CURLINFO_NUM_CONNECTS */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_NUM_CONNECTS,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_SSL_ENGINES
    case 27: /* CURLINFO_SSL_ENGINES */
        resultType = StringListValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_SSL_ENGINES,
                                       &stringListValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_COOKIELIST
    case 28: /* CURLINFO_COOKIELIST */
        resultType = StringListValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_COOKIELIST,
                                       &stringListValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_LASTSOCKET
    case 29: /* CURLINFO_LASTSOCKET */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_LASTSOCKET,
                                       &longValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_FTP_ENTRY_PATH
    case 30: /* CURLINFO_FTP_ENTRY_PATH */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_FTP_ENTRY_PATH,
                                       &strValue);
        break;
#endif

#if HAVE_DECL_CURLINFO_REDIRECT_URL
    case 31: /* CURLINFO_REDIRECT_URL */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_REDIRECT_URL,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_REDIRECT_URL")
#endif

#if HAVE_DECL_CURLINFO_PRIMARY_IP
    case 32: /* CURLINFO_PRIMARY_IP */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_PRIMARY_IP,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_PRIMARY_IP")
#endif

#if HAVE_DECL_CURLINFO_LOCAL_IP
    case 33: /* CURLINFO_LOCAL_IP */
        resultType = StringValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_LOCAL_IP,
                                       &strValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_LOCAL_IP")
#endif

#if HAVE_DECL_CURLINFO_LOCAL_PORT
    case 34: /* CURLINFO_LOCAL_PORT */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_LOCAL_PORT,
                                       &longValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_LOCAL_PORT")
#endif

#if HAVE_DECL_CURLINFO_CONDITION_UNMET
    case 35: /* CURLINFO_CONDITION_UNMET */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CONDITION_UNMET,
                                       &longValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_CONDITION_UNMET")
#endif
#if HAVE_DECL_CURLINFO_CERTINFO
    case 36: /* CURLINFO_CERTINFO */
        resultType = StringListListValue;
        ptr.to_info = NULL;
        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_CERTINFO,
                                       &ptr.to_info);

        result = Val_emptylist;
        current = Val_emptylist;
        next = Val_emptylist;

        if (curlResult != CURLE_OK || !ptr.to_info)
          break;

        for (i = 0; i < ptr.to_certinfo->num_of_certs; i++) {
          next = caml_alloc_tuple(2);
          Store_field(next, 0, convertStringList(ptr.to_certinfo->certinfo[i]));
          Store_field(next, 1, current);
          current = next;
        }
        break;
#else
#pragma message("libcurl does not provide CURLINFO_CERTINFO")
#endif
#if HAVE_DECL_CURLINFO_ACTIVESOCKET
    case 37: /* CURLINFO_ACTIVESOCKET */
        resultType = SocketValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_ACTIVESOCKET,
                                       &socketValue);
        break;
#else
#pragma message("libcurl does not provide CURLINFO_ACTIVESOCKET")
#endif
#if HAVE_DECL_CURLINFO_HTTP_VERSION
    case 38: /* CURLINFO_HTTP_VERSION */
        resultType = LongValue;

        curlResult = curl_easy_getinfo(connection->handle,
                                       CURLINFO_HTTP_VERSION,
                                       &longValue);

        longValue = ocaml_HTTP_VERSION(longValue);

        break;
#else
#pragma message("libcurl does not provide CURLINFO_HTTP_VERSION")
#endif
    /* sync check_enums */
    default:
        caml_failwith("Invalid CURLINFO Option");
        break;
    }

    if (curlResult != CURLE_OK)
        raiseError(connection, curlResult);

    switch (resultType)
    {
    case StringValue:
        result = caml_alloc(1, StringValue);
        Store_field(result, 0, caml_copy_string(strValue?strValue:""));
        break;

    case LongValue:
        result = caml_alloc(1, LongValue);
        Store_field(result, 0, Val_long(longValue));
        break;

    case DoubleValue:
        result = caml_alloc(1, DoubleValue);
        Store_field(result, 0, caml_copy_double(doubleValue));
        break;

    case StringListValue:
        result = caml_alloc(1, StringListValue);
        Store_field(result, 0, convertStringList(stringListValue));
        curl_slist_free_all(stringListValue);
        break;

    case StringListListValue:
        result = caml_alloc(1, StringListListValue);
        Store_field(result, 0, current);
        break;

    case SocketValue:
        result = caml_alloc(1, SocketValue);
        Store_field(result, 0, Val_socket(socketValue));
        break;

    case OCamlValue:
        break;
    }

    CAMLreturn(result);
}

/**
 **  curl_escape helper function
 **/

value caml_curl_escape(value str)
{
    CAMLparam1(str);
    CAMLlocal1(result);
    char *curlResult;

    curlResult = curl_escape(String_val(str), caml_string_length(str));
    result = caml_copy_string(curlResult);
    free(curlResult);

    CAMLreturn(result);
}

/**
 **  curl_unescape helper function
 **/

value caml_curl_unescape(value str)
{
    CAMLparam1(str);
    CAMLlocal1(result);
    char *curlResult;

    curlResult = curl_unescape(String_val(str), caml_string_length(str));
    result = caml_copy_string(curlResult);
    free(curlResult);

    CAMLreturn(result);
}

/**
 **  curl_getdate helper function
 **/

value caml_curl_getdate(value str, value now)
{
    CAMLparam2(str, now);
    CAMLlocal1(result);
    time_t curlResult;
    time_t curlNow;

    curlNow = (time_t)Double_val(now);
    curlResult = curl_getdate(String_val(str), &curlNow);
    result = caml_copy_double((double)curlResult);

    CAMLreturn(result);
}

/**
 **  curl_version helper function
 **/

value caml_curl_version(void)
{
    CAMLparam0();
    CAMLlocal1(result);
    char *str;

    str = curl_version();
    result = caml_copy_string(str);

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

value caml_curl_version_info(value unit)
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

value caml_curl_pause(value conn, value opts)
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

  caml_enter_blocking_section();
  result = curl_easy_pause(connection->handle,bitmask);
  caml_leave_blocking_section();

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

static void raise_multi_error(char const* msg)
{
  static const value* exception = NULL;

  if (NULL == exception)
  {
    exception = caml_named_value("Curl.Multi.Error");
    if (NULL == exception) caml_invalid_argument("Curl.Multi.Error");
  }

  caml_raise_with_string(*exception, msg);
}

static void raise_multi_cerror(char const* func, CURLMcode code)
{
  CAMLparam0();
  CAMLlocal1(data);
  static const value* exception = NULL;

  if (NULL == exception)
  {
    exception = caml_named_value("Curl.Multi.CError");
    if (NULL == exception) caml_invalid_argument("Curl.Multi.CError");
  }

  data = caml_alloc(4, 0);

  Store_field(data, 0, *exception);
  Store_field(data, 1, caml_copy_string(func));
  Store_field(data, 2, Val_int(code));
  Store_field(data, 3, caml_copy_string(curl_multi_strerror(code)));

  caml_raise(data);

  CAMLreturn0;
}

static void check_mcode(char const* func, CURLMcode code)
{
  if (code != CURLM_OK) raise_multi_cerror(func,code);
}

value caml_curl_multi_init(value unit)
{
  CAMLparam1(unit);
  CAMLlocal1(v);
  ml_multi_handle* multi = (ml_multi_handle*)caml_stat_alloc(sizeof(ml_multi_handle));
  CURLM* h = curl_multi_init();

  if (!h)
  {
    caml_stat_free(multi);
    raise_multi_error("caml_curl_multi_init");
  }

  multi->handle = h;
  multi->values = caml_alloc(multi_values_total, 0);
  caml_register_generational_global_root(&multi->values);

  v = caml_alloc_custom(&curl_multi_ops, sizeof(ml_multi_handle*), 0, 1);
  Multi_val(v) = multi;

  CAMLreturn(v);
}

value caml_curl_multi_cleanup(value handle)
{
  CAMLparam1(handle);
  ml_multi_handle* h = Multi_val(handle);

  if (NULL == h)
    CAMLreturn(Val_unit);

  caml_remove_generational_global_root(&h->values);

  CURLcode rc = curl_multi_cleanup(h->handle);

  caml_stat_free(h);
  Multi_val(handle) = (ml_multi_handle*)NULL;

  check_mcode("curl_multi_cleanup",rc);

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
        /*caml_failwith("curlm_remove_finished");*/
      }
      return easy_handle;
    }
  }
}

value caml_curlm_remove_finished(value v_multi)
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
    conn = getConnection(handle);
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

static int curlWait_table[] = {
  CURL_WAIT_POLLIN,
  CURL_WAIT_POLLPRI,
  CURL_WAIT_POLLOUT,
};

static int list_length(value v)
{
  int n = 0;
  while (v != Val_emptylist) {
    n ++;
    v = Field(v, 1);
  }
  return n;
}

static struct curl_waitfd *convert_extra_fds(value v_extra_fds, int *nr_extra_fds)
{
  /* NB this function doesn't trigger GC so can avoid registering local roots */
  value v_extra_fd;

  int nfds = list_length(v_extra_fds);
  *nr_extra_fds = nfds;

  if (nfds == 0) return NULL;

  struct curl_waitfd *extra_fds = caml_stat_alloc(sizeof(extra_fds[0]) * nfds);

  int i = 0;
  while (v_extra_fds != Val_emptylist)
  {
    v_extra_fd = Field(v_extra_fds, 0);
    extra_fds[i].fd = Socket_val(Field(v_extra_fd, 0));
    extra_fds[i].events = caml_convert_flag_list(Field(v_extra_fd, 1), curlWait_table);
    extra_fds[i].revents = 0;
    i ++;
    v_extra_fds = Field(v_extra_fds, 1);
  }

  return extra_fds;
}

/* Assumes v_extra_fds and extra_fds to have the same number of elements */
static void update_extra_fds(value v_extra_fds, struct curl_waitfd *extra_fds)
{
  CAMLparam1(v_extra_fds);
  CAMLlocal2(v_extra_fd, lst);

  int i = 0;
  while (v_extra_fds != Val_emptylist)
  {
    v_extra_fd = Field(v_extra_fds, 0);
    lst = Val_emptylist;
    for (int j = 0; j < sizeof(curlWait_table)/sizeof(curlWait_table[0]); j ++)
    {
      if (curlWait_table[j] & extra_fds[i].revents)
        lst = Val_cons(lst, Val_int(j));
    }
    Store_field(v_extra_fd, 2, lst);
    i ++;
    v_extra_fds = Field(v_extra_fds, 1);
  }

  CAMLreturn0;
}

value caml_curl_multi_wait(value v_timeout_ms, value v_extra_fds, value v_multi)
{
  CAMLparam3(v_timeout_ms,v_extra_fds,v_multi);
  CURLM *multi_handle = CURLM_val(v_multi);
  int timeout_ms = Int_val(v_timeout_ms);
  int numfds = -1;
  int nr_extra_fds = 0;
  struct curl_waitfd *extra_fds = convert_extra_fds(v_extra_fds, &nr_extra_fds);
  CURLMcode rc;

  caml_enter_blocking_section();
  rc = curl_multi_wait(multi_handle, extra_fds, nr_extra_fds, timeout_ms, &numfds);
  caml_leave_blocking_section();

  if (extra_fds != NULL) {
    update_extra_fds(v_extra_fds, extra_fds);
    caml_stat_free(extra_fds);
  }

  check_mcode("curl_multi_wait",rc);

  CAMLreturn(Val_bool(numfds != 0));
}

value caml_curl_multi_poll(value v_timeout_ms, value v_extra_fds, value v_multi)
{
  CAMLparam3(v_timeout_ms,v_extra_fds,v_multi);
  CURLM *multi_handle = CURLM_val(v_multi);
  int timeout_ms = Int_val(v_timeout_ms);
  int numfds = -1;
  int nr_extra_fds = 0;
  struct curl_waitfd *extra_fds = convert_extra_fds(v_extra_fds, &nr_extra_fds);
  CURLMcode rc;

  caml_enter_blocking_section();
#if HAVE_DECL_CURL_MULTI_POLL
  rc = curl_multi_poll(multi_handle, extra_fds, nr_extra_fds, timeout_ms, &numfds);
#else
  rc = curl_multi_wait(multi_handle, extra_fds, nr_extra_fds, timeout_ms, &numfds);
#endif
  caml_leave_blocking_section();

  if (extra_fds != NULL) {
    update_extra_fds(v_extra_fds, extra_fds);
    caml_stat_free(extra_fds);
  }

  check_mcode("curl_multi_poll",rc);

  CAMLreturn(Val_bool(numfds != 0));
}

value caml_curl_multi_add_handle(value v_multi, value v_easy)
{
  CAMLparam2(v_multi,v_easy);
  CURLMcode rc = CURLM_OK;
  CURLM* multi = CURLM_val(v_multi);
  Connection* conn = Connection_val(v_easy);

  /* prevent collection of OCaml value while the easy handle is used
   and may invoke callbacks registered on OCaml side */
  conn->refcount++;

  /* may invoke callbacks so need to be consistent with locks */
  caml_enter_blocking_section();
  rc = curl_multi_add_handle(multi, conn->handle);
  if (CURLM_OK != rc)
  {
    conn->refcount--; /* not added, revert */
    caml_leave_blocking_section();
    check_mcode("curl_multi_add_handle",rc);
  }
  caml_leave_blocking_section();

  CAMLreturn(Val_unit);
}

value caml_curl_multi_remove_handle(value v_multi, value v_easy)
{
  CAMLparam2(v_multi,v_easy);
  CURLM* multi = CURLM_val(v_multi);
  CURLMcode rc = CURLM_OK;
  Connection* conn = Connection_val(v_easy);

  /* may invoke callbacks so need to be consistent with locks */
  caml_enter_blocking_section();
  rc = curl_multi_remove_handle(multi, conn->handle);
  conn->refcount--;
  caml_leave_blocking_section();
  check_mcode("curl_multi_remove_handle",rc);

  CAMLreturn(Val_unit);
}

value caml_curl_multi_perform_all(value v_multi)
{
  CAMLparam1(v_multi);
  int still_running = 0;
  CURLM* h = CURLM_val(v_multi);

  caml_enter_blocking_section();
  while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(h, &still_running));
  caml_leave_blocking_section();

  CAMLreturn(Val_int(still_running));
}

/* currently curlCode repr tag matches CURLE_ value
 * this is exploited below, but generally should use errorMap */

value caml_curl_strerror(value v_code)
{
  CAMLparam1(v_code);
  CAMLreturn(caml_copy_string(curl_easy_strerror((CURLcode)Int_val(v_code))));
}

value caml_curl_int_of_curlCode(value v_code)
{
  return v_code;
}

value caml_curl_curlCode_of_int(value v)
{
  return (Int_val(v) < sizeof(errorMap) / sizeof(errorMap[0]) ? Val_some(v) : Val_none);
}

value caml_curl_multi_socket_action(value v_multi, value v_fd, value v_kind)
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

  check_mcode("curl_multi_socket_action",rc);

  CAMLreturn(Val_int(still_running));
}

value caml_curl_multi_socket_all(value v_multi)
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

  check_mcode("curl_multi_socket_all",rc);

  CAMLreturn(Val_int(still_running));
}

static int curlm_sock_cb(CURL *e, curl_socket_t sock, int what, void *cbp, void *sockp)
{
  caml_leave_blocking_section();

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
      fprintf(stderr, "curlm_sock_cb sock=%lld what=%d\n", (long long)sock, what);
      fflush(stderr);
      raise_multi_error("curlm_sock_cb"); /* FIXME exception from callback */
  }
  csock=Val_socket(sock);
  caml_callback2(Field(((ml_multi_handle*)cbp)->values,curlmopt_socket_function),
                 csock, v_what);
  CAMLdrop;

  caml_enter_blocking_section();
  return 0;
}

value caml_curl_multi_socketfunction(value v_multi, value v_cb)
{
  CAMLparam2(v_multi, v_cb);
  ml_multi_handle* multi = Multi_val(v_multi);

  Store_field(multi->values, curlmopt_socket_function, v_cb);

  curl_multi_setopt(multi->handle, CURLMOPT_SOCKETFUNCTION, curlm_sock_cb);
  curl_multi_setopt(multi->handle, CURLMOPT_SOCKETDATA, multi);

  CAMLreturn(Val_unit);
}

static int curlm_timer_cb(CURLM *multi, long timeout_ms, void *userp)
{
  caml_leave_blocking_section();

  CAMLparam0();
  (void)multi;
  caml_callback(Field(((ml_multi_handle*)userp)->values,curlmopt_timer_function), Val_long(timeout_ms));
  CAMLdrop;

  caml_enter_blocking_section();
  return 0;
}

value caml_curl_multi_timerfunction(value v_multi, value v_cb)
{
  CAMLparam2(v_multi, v_cb);
  ml_multi_handle* multi = Multi_val(v_multi);

  Store_field(multi->values, curlmopt_timer_function, v_cb);

  curl_multi_setopt(multi->handle, CURLMOPT_TIMERFUNCTION, curlm_timer_cb);
  curl_multi_setopt(multi->handle, CURLMOPT_TIMERDATA, multi);

  CAMLreturn(Val_unit);
}

value caml_curl_multi_timeout(value v_multi)
{
  CAMLparam1(v_multi);
  long ms = 0;
  CURLMcode rc = CURLM_OK;
  ml_multi_handle* multi = Multi_val(v_multi);

  rc = curl_multi_timeout(multi->handle, &ms);

  check_mcode("curl_multi_timeout",rc);

  CAMLreturn(Val_long(ms));
}

#define SETMOPT_VAL_(func_name, curl_option, conv_val) \
static void func_name(CURLM *handle, value option) \
{ \
    CAMLparam1(option); \
    CURLMcode result = CURLM_OK; \
\
    result = curl_multi_setopt(handle, curl_option, conv_val(option)); \
\
    check_mcode("curl_multi_setopt "#curl_option,result); \
\
    CAMLreturn0; \
}

#define SETMOPT_VAL(name, conv) SETMOPT_VAL_(handle_multi_##name, CURLMOPT_##name, conv)
#define SETMOPT_BOOL(name) SETMOPT_VAL(name, Bool_val)
#define SETMOPT_LONG(name) SETMOPT_VAL(name, Long_val)
#define SETMOPT_INT64(name) SETMOPT_VAL(name, Int64_val)

long pipeliningMap[] =
{
  0, /* CURLPIPE_NOTHING */
  1, /* CURLPIPE_HTTP1 */
  2, /* CURLPIPE_MULTIPLEX */
};

static void handle_multi_PIPELINING(CURLM* handle, value option)
{
  CAMLparam1(option);
  CURLMcode result = CURLM_OK;

  long bits = convert_bit_list(pipeliningMap, sizeof(pipeliningMap) / sizeof(pipeliningMap[0]), option);

  result = curl_multi_setopt(handle, CURLMOPT_PIPELINING, bits);

  check_mcode("curl_multi_setopt CURLOPT_PIPELINING",result);

  CAMLreturn0;
}

#if HAVE_DECL_CURLMOPT_MAXCONNECTS
SETMOPT_LONG( MAXCONNECTS)
#endif

#if HAVE_DECL_CURLMOPT_MAX_PIPELINE_LENGTH
SETMOPT_LONG( MAX_PIPELINE_LENGTH)
#endif

#if HAVE_DECL_CURLMOPT_MAX_HOST_CONNECTIONS
SETMOPT_LONG( MAX_HOST_CONNECTIONS)
#endif

#if HAVE_DECL_CURLMOPT_MAX_TOTAL_CONNECTIONS
SETMOPT_LONG( MAX_TOTAL_CONNECTIONS)
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
#if HAVE_DECL_CURLMOPT_MAX_TOTAL_CONNECTIONS
  OPT( MAX_TOTAL_CONNECTIONS),
#else
  NO_OPT( MAX_TOTAL_CONNECTIONS),
#endif
};

value caml_curl_multi_setopt(value v_multi, value option)
{
    CAMLparam2(v_multi, option);
    CAMLlocal1(data);
    CURLM *handle = Multi_val(v_multi)->handle;
    CURLMOptionMapping* thisOption = NULL;
    static const value* exception = NULL;

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
      raise_multi_error("Invalid CURLMOPT Option");
    }

    CAMLreturn(Val_unit);
}

struct used_enum
{
  int last_used;
  int last;
  char const* name;
};

#define CURL_ENUM(name,last_used) { CURL_ ## name ## _ ## last_used, CURL_ ## name ## _LAST, #name }

struct used_enum check_enums[] = {
  { CURLINFO_SSL_DATA_OUT, CURLINFO_END, "DEBUGFUNCTION curl_infotype" },
#if HAVE_DECL_CURL_HTTP_VERSION_3
  CURL_ENUM(HTTP_VERSION, 3),
#endif
  { 38, CURLINFO_LASTONE, "CURLINFO" },
#if HAVE_DECL_CURLE_AGAIN
  { CURLE_AGAIN, CURL_LAST, "CURLcode" }
#endif
};

value caml_curl_check_enums(value v_unit)
{
  CAMLparam0();
  CAMLlocal2(v_r,v);
  size_t i;
  size_t len = sizeof(check_enums) / sizeof(struct used_enum);

  v_r = caml_alloc_tuple(len);

  for (i = 0; i < len; i++)
  {
    v = caml_alloc_tuple(3);
    Store_field(v, 0, Val_int(check_enums[i].last_used));
    Store_field(v, 1, Val_int(check_enums[i].last));
    Store_field(v, 2, caml_copy_string(check_enums[i].name));
    Store_field(v_r, i, v);
  }

  CAMLreturn(v_r);
}

#ifdef __cplusplus
}
#endif

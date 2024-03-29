dnl
dnl ocurl autoconf input
dnl

AC_INIT(ocurl,0.9.2)

MIN_LIBCURL_VERSION=7.28.0

AC_PROG_CC()

AC_ARG_VAR([CURLCFLAGS], [libcurl C flags])

AC_MSG_CHECKING([for libcurl cflags])
AS_IF([test -z "$CURLCFLAGS"],
  [CURLCFLAGS=`pkg-config libcurl --cflags || curl-config --cflags`
   AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([libcurl was not found])])])
AC_MSG_RESULT([$CURLCFLAGS])

AC_ARG_VAR([CURLLIBS], [libcurl library flags])

AC_MSG_CHECKING([for libcurl libs])
AS_IF([test -z "$CURLLIBS"],
  [CURLLIBS=`pkg-config libcurl --libs || curl-config --libs`
   AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([libcurl was not found])])])
AC_MSG_RESULT([$CURLLIBS])

AC_ARG_VAR([LIBCURL_VERSION], [libcurl version])

AC_MSG_CHECKING([for libcurl version >= $MIN_LIBCURL_VERSION])
AS_IF([test -z "$LIBCURL_VERSION"],
  [LIBCURL_VERSION=`pkg-config libcurl --modversion || curl-config --version`
   (pkg-config libcurl && pkg-config libcurl --atleast-version=${MIN_LIBCURL_VERSION}) || curl-config --checkfor ${MIN_LIBCURL_VERSION}
   AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([${LIBCURL_VERSION} is too old])])])
AC_MSG_RESULT([${LIBCURL_VERSION}])

# set CFLAGS for tests below
CFLAGS="$CFLAGS $CURLCFLAGS"

AC_CHECK_HEADERS([curl/curl.h],[],[AC_MSG_ERROR([libcurl header file missing])])

AC_CHECK_DECLS([CURLMOPT_TIMERFUNCTION, CURLMOPT_TIMERDATA, CURLMOPT_SOCKETFUNCTION, CURLMOPT_SOCKETDATA],
[], [AC_MSG_ERROR([multi API is too old, need libcurl >= ${MIN_LIBCURL_VERSION}])],
[
#include "curl/curl.h"
])

AC_CHECK_DECLS([
CURLOPT_FILE, CURLOPT_URL, CURLOPT_PORT, CURLOPT_PROXY, CURLOPT_USERPWD,
CURLOPT_PROXYUSERPWD, CURLOPT_RANGE, CURLOPT_INFILE, CURLOPT_ERRORBUFFER,
CURLOPT_WRITEFUNCTION, CURLOPT_READFUNCTION, CURLOPT_TIMEOUT,
CURLOPT_INFILESIZE, CURLOPT_POSTFIELDS, CURLOPT_REFERER, CURLOPT_FTPPORT,
CURLOPT_USERAGENT, CURLOPT_LOW_SPEED_LIMIT, CURLOPT_LOW_SPEED_TIME,
CURLOPT_RESUME_FROM, CURLOPT_COOKIE, CURLOPT_HTTPHEADER,
CURLOPT_HTTPPOST, CURLOPT_SSLCERT, CURLOPT_SSLCERTPASSWD, CURLOPT_SSLKEYPASSWD,
CURLOPT_CRLF, CURLOPT_QUOTE, CURLOPT_WRITEHEADER, CURLOPT_COOKIEFILE,
CURLOPT_SSLVERSION, CURLOPT_TIMECONDITION, CURLOPT_TIMEVALUE,
CURLOPT_CUSTOMREQUEST, CURLOPT_POSTQUOTE, CURLOPT_WRITEINFO,
CURLOPT_VERBOSE, CURLOPT_HEADER, CURLOPT_NOPROGRESS, CURLOPT_NOBODY,
CURLOPT_FAILONERROR, CURLOPT_UPLOAD, CURLOPT_POST, CURLOPT_FTPLISTONLY,
CURLOPT_FTPAPPEND, CURLOPT_NETRC, CURLOPT_FOLLOWLOCATION,
CURLOPT_TRANSFERTEXT, CURLOPT_PUT, CURLOPT_PROGRESSFUNCTION, CURLOPT_XFERINFOFUNCTION,
CURLOPT_PREREQFUNCTION, CURLOPT_PROGRESSDATA, CURLOPT_AUTOREFERER, CURLOPT_PROXYPORT,
CURLOPT_POSTFIELDSIZE, CURLOPT_HTTPPROXYTUNNEL, CURLOPT_INTERFACE,
CURLOPT_KRB4LEVEL, CURLOPT_SSL_VERIFYPEER, CURLOPT_CAINFO,
CURLOPT_MAXREDIRS, CURLOPT_FILETIME, CURLOPT_TELNETOPTIONS,
CURLOPT_MAXCONNECTS, CURLOPT_CLOSEPOLICY, CURLOPT_FRESH_CONNECT,
CURLOPT_FORBID_REUSE, CURLOPT_RANDOM_FILE, CURLOPT_EGDSOCKET,
CURLOPT_CONNECTTIMEOUT, CURLOPT_HEADERFUNCTION, CURLOPT_HTTPGET,
CURLOPT_SSL_VERIFYHOST, CURLOPT_COOKIEJAR, CURLOPT_SSL_CIPHER_LIST,
CURLOPT_HTTP_VERSION, CURLOPT_FTP_USE_EPSV, CURLOPT_SSLCERTTYPE,
CURLOPT_SSLKEY, CURLOPT_SSLKEYTYPE, CURLOPT_SSLENGINE,
CURLOPT_SSLENGINE_DEFAULT, CURLOPT_DNS_USE_GLOBAL_CACHE,
CURLOPT_DNS_CACHE_TIMEOUT, CURLOPT_PREQUOTE, CURLOPT_DEBUGFUNCTION,
CURLOPT_DEBUGDATA, CURLOPT_COOKIESESSION, CURLOPT_CAPATH,
CURLOPT_BUFFERSIZE, CURLOPT_NOSIGNAL, CURLOPT_SHARE,
CURLOPT_PROXYTYPE, CURLOPT_ENCODING, CURLOPT_HTTP200ALIASES,
CURLOPT_UNRESTRICTED_AUTH, CURLOPT_FTP_USE_EPRT, CURLOPT_HTTPAUTH,
CURLOPT_FTP_CREATE_MISSING_DIRS, CURLOPT_PROXYAUTH,
CURLOPT_FTP_RESPONSE_TIMEOUT, CURLOPT_IPRESOLVE, CURLOPT_MAXFILESIZE,
CURLOPT_INFILESIZE_LARGE, CURLOPT_RESUME_FROM_LARGE,
CURLOPT_MAXFILESIZE_LARGE, CURLOPT_NETRC_FILE, CURLOPT_FTP_SSL,
CURLOPT_POSTFIELDSIZE_LARGE, CURLOPT_TCP_NODELAY, CURLOPT_FTPSSLAUTH,
CURLOPT_IOCTLFUNCTION, CURLOPT_FTP_ACCOUNT, CURLOPT_COOKIELIST,
CURLOPT_IGNORE_CONTENT_LENGTH, CURLOPT_FTP_SKIP_PASV_IP,
CURLOPT_FTP_FILEMETHOD, CURLOPT_LOCALPORT, CURLOPT_LOCALPORTRANGE,
CURLOPT_CONNECT_ONLY, CURLOPT_MAX_SEND_SPEED_LARGE,
CURLOPT_MAX_RECV_SPEED_LARGE, CURLOPT_FTP_ALTERNATIVE_TO_USER,
CURLOPT_SSL_SESSIONID_CACHE, CURLOPT_SSH_AUTH_TYPES,
CURLOPT_SSH_PUBLIC_KEYFILE, CURLOPT_SSH_PRIVATE_KEYFILE,
CURLOPT_FTP_SSL_CCC, CURLOPT_TIMEOUT_MS, CURLOPT_CONNECTTIMEOUT_MS,
CURLOPT_HTTP_TRANSFER_DECODING, CURLOPT_HTTP_CONTENT_DECODING,
CURLOPT_NEW_FILE_PERMS, CURLOPT_NEW_DIRECTORY_PERMS, CURLOPT_POST301,
CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, CURLOPT_COPYPOSTFIELDS,
CURLOPT_PROXY_TRANSFER_MODE, CURLOPT_SEEKFUNCTION,
CURLOPT_PROTOCOLS, CURLOPT_REDIR_PROTOCOLS, CURLOPT_RESOLVE, CURLOPT_DNS_SERVERS,
CURLOPT_MAIL_FROM, CURLOPT_MAIL_RCPT, CURLOPT_PIPEWAIT, CURLOPT_CERTINFO,
CURLOPT_USERNAME, CURLOPT_PASSWORD, CURLOPT_LOGIN_OPTIONS, CURLOPT_CONNECT_TO,
CURLOPT_POSTREDIR, CURLOPT_MIMEPOST, CURLOPT_BUFFERSIZE, CURLOPT_DOH_URL,
CURLOPT_TCP_FASTOPEN, CURLOPT_AWS_SIGV4,

CURLINFO_EFFECTIVE_URL, CURLINFO_RESPONSE_CODE, CURLINFO_TOTAL_TIME,
CURLINFO_NAMELOOKUP_TIME, CURLINFO_CONNECT_TIME, CURLINFO_PRETRANSFER_TIME,
CURLINFO_SIZE_UPLOAD, CURLINFO_SIZE_DOWNLOAD, CURLINFO_SPEED_DOWNLOAD,
CURLINFO_SPEED_UPLOAD, CURLINFO_HEADER_SIZE, CURLINFO_REQUEST_SIZE,
CURLINFO_SSL_VERIFYRESULT, CURLINFO_FILETIME, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
CURLINFO_CONTENT_LENGTH_UPLOAD, CURLINFO_STARTTRANSFER_TIME,
CURLINFO_CONTENT_TYPE, CURLINFO_REDIRECT_TIME, CURLINFO_REDIRECT_COUNT,
CURLINFO_HTTP_CONNECTCODE, CURLINFO_HTTPAUTH_AVAIL,
CURLINFO_PROXYAUTH_AVAIL, CURLINFO_OS_ERRNO, CURLINFO_NUM_CONNECTS,
CURLINFO_SSL_ENGINES, CURLINFO_COOKIELIST, CURLINFO_LASTSOCKET, CURLINFO_ACTIVESOCKET,
CURLINFO_FTP_ENTRY_PATH, CURLINFO_REDIRECT_URL, CURLINFO_PRIMARY_IP,
CURLINFO_LOCAL_IP, CURLINFO_LOCAL_PORT, CURLINFO_CONDITION_UNMET, CURLINFO_CERTINFO,
CURLINFO_HTTP_VERSION,

CURLE_UNSUPPORTED_PROTOCOL, CURLE_FAILED_INIT, CURLE_URL_MALFORMAT,
CURLE_URL_MALFORMAT_USER, CURLE_COULDNT_RESOLVE_PROXY,
CURLE_COULDNT_RESOLVE_HOST, CURLE_COULDNT_CONNECT,
CURLE_FTP_WEIRD_SERVER_REPLY, CURLE_FTP_ACCESS_DENIED,
CURLE_FTP_USER_PASSWORD_INCORRECT, CURLE_FTP_WEIRD_PASS_REPLY,
CURLE_FTP_WEIRD_USER_REPLY, CURLE_FTP_WEIRD_PASV_REPLY,
CURLE_FTP_WEIRD_227_FORMAT, CURLE_FTP_CANT_GET_HOST, CURLE_FTP_CANT_RECONNECT,
CURLE_FTP_COULDNT_SET_BINARY, CURLE_PARTIAL_FILE, CURLE_FTP_COULDNT_RETR_FILE,
CURLE_FTP_WRITE_ERROR, CURLE_FTP_QUOTE_ERROR, CURLE_HTTP_RETURNED_ERROR,
CURLE_WRITE_ERROR, CURLE_MALFORMAT_USER, CURLE_FTP_COULDNT_STOR_FILE,
CURLE_READ_ERROR, CURLE_OUT_OF_MEMORY, CURLE_OPERATION_TIMEOUTED,
CURLE_FTP_COULDNT_SET_ASCII, CURLE_FTP_PORT_FAILED, CURLE_FTP_COULDNT_USE_REST,
CURLE_FTP_COULDNT_GET_SIZE, CURLE_HTTP_RANGE_ERROR, CURLE_HTTP_POST_ERROR,
CURLE_SSL_CONNECT_ERROR, CURLE_BAD_DOWNLOAD_RESUME,
CURLE_FILE_COULDNT_READ_FILE, CURLE_LDAP_CANNOT_BIND, CURLE_LDAP_SEARCH_FAILED,
CURLE_LIBRARY_NOT_FOUND, CURLE_FUNCTION_NOT_FOUND, CURLE_ABORTED_BY_CALLBACK,
CURLE_BAD_FUNCTION_ARGUMENT, CURLE_BAD_CALLING_ORDER, CURLE_INTERFACE_FAILED,
CURLE_BAD_PASSWORD_ENTERED, CURLE_TOO_MANY_REDIRECTS,
CURLE_UNKNOWN_TELNET_OPTION, CURLE_TELNET_OPTION_SYNTAX,
CURLE_SSL_PEER_CERTIFICATE, CURLE_GOT_NOTHING, CURLE_SSL_ENGINE_NOTFOUND,
CURLE_SSL_ENGINE_SETFAILED, CURLE_SEND_ERROR, CURLE_RECV_ERROR,
CURLE_SHARE_IN_USE, CURLE_SSL_CERTPROBLEM, CURLE_SSL_CIPHER, CURLE_SSL_CACERT,
CURLE_BAD_CONTENT_ENCODING, CURLE_LDAP_INVALID_URL, CURLE_FILESIZE_EXCEEDED,
CURLE_FTP_SSL_FAILED, CURLE_SEND_FAIL_REWIND, CURLE_SSL_ENGINE_INITFAILED,
CURLE_LOGIN_DENIED, CURLE_TFTP_NOTFOUND, CURLE_TFTP_PERM,
CURLE_REMOTE_DISK_FULL, CURLE_TFTP_ILLEGAL, CURLE_TFTP_UNKNOWNID,
CURLE_REMOTE_FILE_EXISTS, CURLE_TFTP_NOSUCHUSER, CURLE_CONV_FAILED,
CURLE_CONV_REQD, CURLE_SSL_CACERT_BADFILE, CURLE_REMOTE_FILE_NOT_FOUND,
CURLE_SSH, CURLE_SSL_SHUTDOWN_FAILED, CURLE_AGAIN,

CURLMOPT_MAXCONNECTS, CURLMOPT_PIPELINING, CURLMOPT_MAX_PIPELINE_LENGTH,
CURLMOPT_MAX_HOST_CONNECTIONS, CURLMOPT_MAX_TOTAL_CONNECTIONS,

CURL_SSLVERSION_TLSv1_0, CURL_SSLVERSION_TLSv1_1, CURL_SSLVERSION_TLSv1_2, CURL_SSLVERSION_TLSv1_3,

CURL_HTTP_VERSION_2, CURL_HTTP_VERSION_2_0, CURL_HTTP_VERSION_2TLS,
CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE, CURL_HTTP_VERSION_3,

CURLSSLBACKEND_NONE, CURLSSLBACKEND_OPENSSL, CURLSSLBACKEND_GNUTLS, CURLSSLBACKEND_NSS,
CURLSSLBACKEND_GSKIT, CURLSSLBACKEND_WOLFSSL, CURLSSLBACKEND_SCHANNEL, CURLSSLBACKEND_SECURETRANSPORT,
CURLSSLBACKEND_MBEDTLS, CURLSSLBACKEND_MESALINK, CURLSSLBACKEND_BEARSSL,

CURLOPT_SSL_OPTIONS,

CURL_VERSION_TLSAUTH_SRP, CURL_VERSION_NTLM_WB],
[], [],
[
#include "curl/curl.h"
])

AC_CHECK_DECLS([curl_multi_poll, curl_global_sslset],
[], [],
[
#include "curl/curl.h"
])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([clibs.sexp])
AC_CONFIG_FILES([cflags.sexp])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([dune-project])

AC_OUTPUT

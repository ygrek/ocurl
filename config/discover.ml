module C = Configurator.V1

let declarations = [
  "CURLE_ABORTED_BY_CALLBACK";
  "CURLE_AGAIN";
  "CURLE_BAD_CALLING_ORDER";
  "CURLE_BAD_CONTENT_ENCODING";
  "CURLE_BAD_DOWNLOAD_RESUME";
  "CURLE_BAD_FUNCTION_ARGUMENT";
  "CURLE_BAD_PASSWORD_ENTERED";
  "CURLE_CONV_FAILED";
  "CURLE_CONV_REQD";
  "CURLE_COULDNT_CONNECT";
  "CURLE_COULDNT_RESOLVE_HOST";
  "CURLE_COULDNT_RESOLVE_PROXY";
  "CURLE_FAILED_INIT";
  "CURLE_FILESIZE_EXCEEDED";
  "CURLE_FILE_COULDNT_READ_FILE";
  "CURLE_FTP_ACCESS_DENIED";
  "CURLE_FTP_CANT_GET_HOST";
  "CURLE_FTP_CANT_RECONNECT";
  "CURLE_FTP_COULDNT_GET_SIZE";
  "CURLE_FTP_COULDNT_RETR_FILE";
  "CURLE_FTP_COULDNT_SET_ASCII";
  "CURLE_FTP_COULDNT_SET_BINARY";
  "CURLE_FTP_COULDNT_STOR_FILE";
  "CURLE_FTP_COULDNT_USE_REST";
  "CURLE_FTP_PORT_FAILED";
  "CURLE_FTP_QUOTE_ERROR";
  "CURLE_FTP_SSL_FAILED";
  "CURLE_FTP_USER_PASSWORD_INCORRECT";
  "CURLE_FTP_WEIRD_227_FORMAT";
  "CURLE_FTP_WEIRD_PASS_REPLY";
  "CURLE_FTP_WEIRD_PASV_REPLY";
  "CURLE_FTP_WEIRD_SERVER_REPLY";
  "CURLE_FTP_WEIRD_USER_REPLY";
  "CURLE_FTP_WRITE_ERROR";
  "CURLE_FUNCTION_NOT_FOUND";
  "CURLE_GOT_NOTHING";
  "CURLE_HTTP_POST_ERROR";
  "CURLE_HTTP_RANGE_ERROR";
  "CURLE_HTTP_RETURNED_ERROR";
  "CURLE_INTERFACE_FAILED";
  "CURLE_LDAP_CANNOT_BIND";
  "CURLE_LDAP_INVALID_URL";
  "CURLE_LDAP_SEARCH_FAILED";
  "CURLE_LIBRARY_NOT_FOUND";
  "CURLE_LOGIN_DENIED";
  "CURLE_MALFORMAT_USER";
  "CURLE_OPERATION_TIMEOUTED";
  "CURLE_OUT_OF_MEMORY";
  "CURLE_PARTIAL_FILE";
  "CURLE_READ_ERROR";
  "CURLE_RECV_ERROR";
  "CURLE_REMOTE_DISK_FULL";
  "CURLE_REMOTE_FILE_EXISTS";
  "CURLE_REMOTE_FILE_NOT_FOUND";
  "CURLE_SEND_ERROR";
  "CURLE_SEND_FAIL_REWIND";
  "CURLE_SHARE_IN_USE";
  "CURLE_SSH";
  "CURLE_SSL_CACERT";
  "CURLE_SSL_CACERT_BADFILE";
  "CURLE_SSL_CERTPROBLEM";
  "CURLE_SSL_CIPHER";
  "CURLE_SSL_CONNECT_ERROR";
  "CURLE_SSL_ENGINE_INITFAILED";
  "CURLE_SSL_ENGINE_NOTFOUND";
  "CURLE_SSL_ENGINE_SETFAILED";
  "CURLE_SSL_PEER_CERTIFICATE";
  "CURLE_SSL_SHUTDOWN_FAILED";
  "CURLE_TELNET_OPTION_SYNTAX";
  "CURLE_TFTP_ILLEGAL";
  "CURLE_TFTP_NOSUCHUSER";
  "CURLE_TFTP_NOTFOUND";
  "CURLE_TFTP_PERM";
  "CURLE_TFTP_UNKNOWNID";
  "CURLE_TOO_MANY_REDIRECTS";
  "CURLE_UNKNOWN_TELNET_OPTION";
  "CURLE_UNSUPPORTED_PROTOCOL";
  "CURLE_URL_MALFORMAT";
  "CURLE_URL_MALFORMAT_USER";
  "CURLE_WRITE_ERROR";

  "CURLINFO_ACTIVESOCKET";
  "CURLINFO_CERTINFO";
  "CURLINFO_CONDITION_UNMET";
  "CURLINFO_CONNECT_TIME";
  "CURLINFO_CONTENT_LENGTH_DOWNLOAD";
  "CURLINFO_CONTENT_LENGTH_UPLOAD";
  "CURLINFO_CONTENT_TYPE";
  "CURLINFO_COOKIELIST";
  "CURLINFO_EFFECTIVE_URL";
  "CURLINFO_FILETIME";
  "CURLINFO_FTP_ENTRY_PATH";
  "CURLINFO_HEADER_SIZE";
  "CURLINFO_HTTPAUTH_AVAIL";
  "CURLINFO_HTTP_CONNECTCODE";
  "CURLINFO_HTTP_VERSION";
  "CURLINFO_LASTSOCKET";
  "CURLINFO_LOCAL_IP";
  "CURLINFO_LOCAL_PORT";
  "CURLINFO_NAMELOOKUP_TIME";
  "CURLINFO_NUM_CONNECTS";
  "CURLINFO_OS_ERRNO";
  "CURLINFO_PRETRANSFER_TIME";
  "CURLINFO_PRIMARY_IP";
  "CURLINFO_PROXYAUTH_AVAIL";
  "CURLINFO_REDIRECT_COUNT";
  "CURLINFO_REDIRECT_TIME";
  "CURLINFO_REDIRECT_URL";
  "CURLINFO_REQUEST_SIZE";
  "CURLINFO_RESPONSE_CODE";
  "CURLINFO_SIZE_DOWNLOAD";
  "CURLINFO_SIZE_UPLOAD";
  "CURLINFO_SPEED_DOWNLOAD";
  "CURLINFO_SPEED_UPLOAD";
  "CURLINFO_SSL_ENGINES";
  "CURLINFO_SSL_VERIFYRESULT";
  "CURLINFO_STARTTRANSFER_TIME";
  "CURLINFO_TOTAL_TIME";

  "CURLMOPT_MAXCONNECTS";
  "CURLMOPT_MAX_HOST_CONNECTIONS";
  "CURLMOPT_MAX_PIPELINE_LENGTH";
  "CURLMOPT_MAX_TOTAL_CONNECTIONS";
  "CURLMOPT_PIPELINING";
  "CURLMOPT_SOCKETDATA";
  "CURLMOPT_SOCKETFUNCTION";
  "CURLMOPT_TIMERDATA";
  "CURLMOPT_TIMERFUNCTION";

  "CURLOPT_AUTOREFERER";
  "CURLOPT_AWS_SIGV4";
  "CURLOPT_BUFFERSIZE";
  "CURLOPT_CAINFO";
  "CURLOPT_CAPATH";
  "CURLOPT_CERTINFO";
  "CURLOPT_CLOSEPOLICY";
  "CURLOPT_CONNECTTIMEOUT";
  "CURLOPT_CONNECTTIMEOUT_MS";
  "CURLOPT_CONNECT_ONLY";
  "CURLOPT_CONNECT_TO";
  "CURLOPT_COOKIE";
  "CURLOPT_COOKIEFILE";
  "CURLOPT_COOKIEJAR";
  "CURLOPT_COOKIELIST";
  "CURLOPT_COOKIESESSION";
  "CURLOPT_COPYPOSTFIELDS";
  "CURLOPT_CRLF";
  "CURLOPT_CUSTOMREQUEST";
  "CURLOPT_DEBUGDATA";
  "CURLOPT_DEBUGFUNCTION";
  "CURLOPT_DNS_CACHE_TIMEOUT";
  "CURLOPT_DNS_SERVERS";
  "CURLOPT_DNS_USE_GLOBAL_CACHE";
  "CURLOPT_DOH_URL";
  "CURLOPT_EGDSOCKET";
  "CURLOPT_ENCODING";
  "CURLOPT_ERRORBUFFER";
  "CURLOPT_FAILONERROR";
  "CURLOPT_FILE";
  "CURLOPT_FILETIME";
  "CURLOPT_FOLLOWLOCATION";
  "CURLOPT_FORBID_REUSE";
  "CURLOPT_FRESH_CONNECT";
  "CURLOPT_FTPAPPEND";
  "CURLOPT_FTPLISTONLY";
  "CURLOPT_FTPPORT";
  "CURLOPT_FTPSSLAUTH";
  "CURLOPT_FTP_ACCOUNT";
  "CURLOPT_FTP_ALTERNATIVE_TO_USER";
  "CURLOPT_FTP_CREATE_MISSING_DIRS";
  "CURLOPT_FTP_FILEMETHOD";
  "CURLOPT_FTP_RESPONSE_TIMEOUT";
  "CURLOPT_FTP_SKIP_PASV_IP";
  "CURLOPT_FTP_SSL";
  "CURLOPT_FTP_SSL_CCC";
  "CURLOPT_FTP_USE_EPRT";
  "CURLOPT_FTP_USE_EPSV";
  "CURLOPT_HEADER";
  "CURLOPT_HEADERFUNCTION";
  "CURLOPT_HTTP200ALIASES";
  "CURLOPT_HTTPAUTH";
  "CURLOPT_HTTPGET";
  "CURLOPT_HTTPHEADER";
  "CURLOPT_HTTPPOST";
  "CURLOPT_HTTPPROXYTUNNEL";
  "CURLOPT_HTTP_CONTENT_DECODING";
  "CURLOPT_HTTP_TRANSFER_DECODING";
  "CURLOPT_HTTP_VERSION";
  "CURLOPT_IGNORE_CONTENT_LENGTH";
  "CURLOPT_INFILE";
  "CURLOPT_INFILESIZE";
  "CURLOPT_INFILESIZE_LARGE";
  "CURLOPT_INTERFACE";
  "CURLOPT_IOCTLFUNCTION";
  "CURLOPT_IPRESOLVE";
  "CURLOPT_KRB4LEVEL";
  "CURLOPT_LOCALPORT";
  "CURLOPT_LOCALPORTRANGE";
  "CURLOPT_LOGIN_OPTIONS";
  "CURLOPT_LOW_SPEED_LIMIT";
  "CURLOPT_LOW_SPEED_TIME";
  "CURLOPT_MAIL_FROM";
  "CURLOPT_MAIL_RCPT";
  "CURLOPT_MAXCONNECTS";
  "CURLOPT_MAXFILESIZE";
  "CURLOPT_MAXFILESIZE_LARGE";
  "CURLOPT_MAXREDIRS";
  "CURLOPT_MAX_RECV_SPEED_LARGE";
  "CURLOPT_MAX_SEND_SPEED_LARGE";
  "CURLOPT_MIMEPOST";
  "CURLOPT_NETRC";
  "CURLOPT_NETRC_FILE";
  "CURLOPT_NEW_DIRECTORY_PERMS";
  "CURLOPT_NEW_FILE_PERMS";
  "CURLOPT_NOBODY";
  "CURLOPT_NOPROGRESS";
  "CURLOPT_NOSIGNAL";
  "CURLOPT_PASSWORD";
  "CURLOPT_PIPEWAIT";
  "CURLOPT_PORT";
  "CURLOPT_POST";
  "CURLOPT_POST301";
  "CURLOPT_POSTFIELDS";
  "CURLOPT_POSTFIELDSIZE";
  "CURLOPT_POSTFIELDSIZE_LARGE";
  "CURLOPT_POSTQUOTE";
  "CURLOPT_POSTREDIR";
  "CURLOPT_PREQUOTE";
  "CURLOPT_PREREQFUNCTION";
  "CURLOPT_PROGRESSDATA";
  "CURLOPT_PROGRESSFUNCTION";
  "CURLOPT_PROTOCOLS";
  "CURLOPT_PROXY";
  "CURLOPT_PROXYAUTH";
  "CURLOPT_PROXYPORT";
  "CURLOPT_PROXYTYPE";
  "CURLOPT_PROXYUSERPWD";
  "CURLOPT_PROXY_TRANSFER_MODE";
  "CURLOPT_PUT";
  "CURLOPT_QUOTE";
  "CURLOPT_RANDOM_FILE";
  "CURLOPT_RANGE";
  "CURLOPT_READFUNCTION";
  "CURLOPT_REDIR_PROTOCOLS";
  "CURLOPT_REFERER";
  "CURLOPT_RESOLVE";
  "CURLOPT_RESUME_FROM";
  "CURLOPT_RESUME_FROM_LARGE";
  "CURLOPT_SEEKFUNCTION";
  "CURLOPT_SHARE";
  "CURLOPT_SSH_AUTH_TYPES";
  "CURLOPT_SSH_HOST_PUBLIC_KEY_MD5";
  "CURLOPT_SSH_PRIVATE_KEYFILE";
  "CURLOPT_SSH_PUBLIC_KEYFILE";
  "CURLOPT_SSLCERT";
  "CURLOPT_SSLCERTPASSWD";
  "CURLOPT_SSLCERTTYPE";
  "CURLOPT_SSLENGINE";
  "CURLOPT_SSLENGINE_DEFAULT";
  "CURLOPT_SSLKEY";
  "CURLOPT_SSLKEYPASSWD";
  "CURLOPT_SSLKEYTYPE";
  "CURLOPT_SSLVERSION";
  "CURLOPT_SSL_CIPHER_LIST";
  "CURLOPT_SSL_OPTIONS";
  "CURLOPT_SSL_SESSIONID_CACHE";
  "CURLOPT_SSL_VERIFYHOST";
  "CURLOPT_SSL_VERIFYPEER";
  "CURLOPT_TCP_FASTOPEN";
  "CURLOPT_TCP_NODELAY";
  "CURLOPT_TELNETOPTIONS";
  "CURLOPT_TIMECONDITION";
  "CURLOPT_TIMEOUT";
  "CURLOPT_TIMEOUT_MS";
  "CURLOPT_TIMEVALUE";
  "CURLOPT_TRANSFERTEXT";
  "CURLOPT_UNRESTRICTED_AUTH";
  "CURLOPT_UPLOAD";
  "CURLOPT_URL";
  "CURLOPT_USERAGENT";
  "CURLOPT_USERNAME";
  "CURLOPT_USERPWD";
  "CURLOPT_VERBOSE";
  "CURLOPT_WRITEFUNCTION";
  "CURLOPT_WRITEHEADER";
  "CURLOPT_WRITEINFO";
  "CURLOPT_XFERINFOFUNCTION";

  "CURLSSLBACKEND_BEARSSL";
  "CURLSSLBACKEND_GNUTLS";
  "CURLSSLBACKEND_GSKIT";
  "CURLSSLBACKEND_MBEDTLS";
  "CURLSSLBACKEND_MESALINK";
  "CURLSSLBACKEND_NONE";
  "CURLSSLBACKEND_NSS";
  "CURLSSLBACKEND_OPENSSL";
  "CURLSSLBACKEND_SCHANNEL";
  "CURLSSLBACKEND_SECURETRANSPORT";
  "CURLSSLBACKEND_WOLFSSL";

  "CURL_HTTP_VERSION_2";
  "CURL_HTTP_VERSION_2TLS";
  "CURL_HTTP_VERSION_2_0";
  "CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE";
  "CURL_HTTP_VERSION_3";

  "CURL_SSLVERSION_TLSv1_0";
  "CURL_SSLVERSION_TLSv1_1";
  "CURL_SSLVERSION_TLSv1_2";
  "CURL_SSLVERSION_TLSv1_3";

  "CURL_VERSION_NTLM_WB";
  "CURL_VERSION_TLSAUTH_SRP";

  "curl_multi_poll";
  "curl_global_sslset";
]

let extract_declarations c ~cflags ~libs =
  let c_source decls =
    let lines =
      [
        "#include <curl/curl.h>";
        "int main()";
        "{";
      ]
      @
      List.concat_map (fun s -> [ "#ifndef " ^ s; "(void)" ^ s ^ ";"; "#endif" ]) decls
      @
      [
        "return 0;";
        "}";
      ]
    in
    String.concat "\n" lines
  in
  let decls =
    (* As an optimization, first we check if *all* declarations are supported;
       this is much faster than checking one by one. *)
    let c_code = c_source declarations in
    if C.c_test c ~c_flags:cflags ~link_flags:libs c_code then
      List.map (fun decl -> decl, true) declarations
    else
      (* Otherwise, we check one by one. *)
      let check decl =
        let c_code = c_source [ decl ] in
        decl, C.c_test c ~c_flags:cflags ~link_flags:libs c_code
      in
      List.map check declarations
  in
  List.map (fun (decl, exists) ->
      "HAVE_DECL_" ^ String.uppercase_ascii decl,
      C.C_define.Value.Int (if exists then 1 else 0)
    ) decls

let main c =
  let {C.Pkg_config.cflags; libs} =
    match C.Pkg_config.get c with
    | None ->
      { C.Pkg_config.libs = [ "-lcurl" ]; cflags = [] }
    | Some pc ->
      (match C.Pkg_config.query_expr_err pc ~package:"libcurl" ~expr:"libcurl >= 7.28.0" with
       | Ok c ->  c
       | Error err -> C.die "%s" err)
  in
  let cflags =
    match C.ocaml_config_var c "ccomp_type" with
    | Some "cc" -> "-Wno-deprecated-declarations" :: cflags, []
    | Some "msvc" -> cflags, ["-defaultlib"; "ws2_32.lib"]
    | _ -> cflags
  in
  C.C_define.gen_header_file c ~fname:"config.h" (extract_declarations c ~cflags ~libs);
  C.Flags.write_sexp "cflags.sexp" cflags;
  C.Flags.write_sexp "clibs.sexp" libs

let () =
  C.main ~name:"ocurl" main

let all = [
  "CURLE_OK";
  "CURLE_UNSUPPORTED_PROTOCOL";
  "CURLE_FAILED_INIT";
  "CURLE_URL_MALFORMAT";
  "CURLE_URL_MALFORMAT_USER";
  "CURLE_COULDNT_RESOLVE_PROXY";
  "CURLE_COULDNT_RESOLVE_HOST";
  "CURLE_COULDNT_CONNECT";
  "CURLE_FTP_WEIRD_SERVER_REPLY";
  "CURLE_FTP_ACCESS_DENIED";
  "CURLE_FTP_USER_PASSWORD_INCORRECT";
  "CURLE_FTP_WEIRD_PASS_REPLY";
  "CURLE_FTP_WEIRD_USER_REPLY";
  "CURLE_FTP_WEIRD_PASV_REPLY";
  "CURLE_FTP_WEIRD_227_FORMAT";
  "CURLE_FTP_CANT_GET_HOST";
  "CURLE_FTP_CANT_RECONNECT";
  "CURLE_FTP_COULDNT_SET_BINARY";
  "CURLE_PARTIAL_FILE";
  "CURLE_FTP_COULDNT_RETR_FILE";
  "CURLE_FTP_WRITE_ERROR";
  "CURLE_FTP_QUOTE_ERROR";
  "CURLE_HTTP_RETURNED_ERROR";
  "CURLE_WRITE_ERROR";
  "CURLE_MALFORMAT_USER";
  "CURLE_FTP_COULDNT_STOR_FILE";
  "CURLE_READ_ERROR";
  "CURLE_OUT_OF_MEMORY";
  "CURLE_OPERATION_TIMEOUTED";
  "CURLE_FTP_COULDNT_SET_ASCII";
  "CURLE_FTP_PORT_FAILED";
  "CURLE_FTP_COULDNT_USE_REST";
  "CURLE_FTP_COULDNT_GET_SIZE";
  "CURLE_HTTP_RANGE_ERROR";
  "CURLE_HTTP_POST_ERROR";
  "CURLE_SSL_CONNECT_ERROR";
  "CURLE_BAD_DOWNLOAD_RESUME";
  "CURLE_FILE_COULDNT_READ_FILE";
  "CURLE_LDAP_CANNOT_BIND";
  "CURLE_LDAP_SEARCH_FAILED";
  "CURLE_LIBRARY_NOT_FOUND";
  "CURLE_FUNCTION_NOT_FOUND";
  "CURLE_ABORTED_BY_CALLBACK";
  "CURLE_BAD_FUNCTION_ARGUMENT";
  "CURLE_BAD_CALLING_ORDER";
  "CURLE_INTERFACE_FAILED";
  "CURLE_BAD_PASSWORD_ENTERED";
  "CURLE_TOO_MANY_REDIRECTS";
  "CURLE_UNKNOWN_TELNET_OPTION";
  "CURLE_TELNET_OPTION_SYNTAX";
  "CURLE_OBSOLETE";
  "CURLE_SSL_PEER_CERTIFICATE";
  "CURLE_GOT_NOTHING";
  "CURLE_SSL_ENGINE_NOTFOUND";
  "CURLE_SSL_ENGINE_SETFAILED";
  "CURLE_SEND_ERROR";
  "CURLE_RECV_ERROR";
  "CURLE_SHARE_IN_USE";
  "CURLE_SSL_CERTPROBLEM";
  "CURLE_SSL_CIPHER";
  "CURLE_SSL_CACERT";
  "CURLE_BAD_CONTENT_ENCODING";
  "CURLE_LDAP_INVALID_URL";
  "CURLE_FILESIZE_EXCEEDED";
  "CURLE_FTP_SSL_FAILED";
  "CURLE_SEND_FAIL_REWIND";
  "CURLE_SSL_ENGINE_INITFAILED";
  "CURLE_LOGIN_DENIED";
  "CURLE_TFTP_NOTFOUND";
  "CURLE_TFTP_PERM";
  "CURLE_REMOTE_DISK_FULL";
  "CURLE_TFTP_ILLEGAL";
  "CURLE_TFTP_UNKNOWNID";
  "CURLE_REMOTE_FILE_EXISTS";
  "CURLE_TFTP_NOSUCHUSER";
  "CURLE_CONV_FAILED";
  "CURLE_CONV_REQD";
  "CURLE_SSL_CACERT_BADFILE";
  "CURLE_REMOTE_FILE_NOT_FOUND";
  "CURLE_SSH";
  "CURLE_SSL_SHUTDOWN_FAILED";
  "CURLE_AGAIN";
  "CURLE_SSL_CRL_BADFILE";
  "CURLE_SSL_ISSUER_ERROR";
  "CURLE_FTP_PRET_FAILED";
  "CURLE_RTSP_CSEQ_ERROR";
  "CURLE_RTSP_SESSION_ERROR";
  "CURLE_FTP_BAD_FILE_LIST";
  "CURLE_CHUNK_FAILED";
  "CURLE_NO_CONNECTION_AVAILABLE";
  "CURLE_SSL_PINNEDPUBKEYNOTMATCH";
  "CURLE_SSL_INVALIDCERTSTATUS";
  "CURLE_HTTP2_STREAM";
  "CURLE_RECURSIVE_API_CALL";
  "CURLE_AUTH_ERROR";
  "CURLE_HTTP3";
  "CURLE_QUIC_CONNECT_ERROR";
  "CURLE_PROXY";
  "CURLE_SSL_CLIENTCERT";
  "CURLE_UNRECOVERABLE_POLL";
  "CURLE_TOO_LARGE";
  "CURLE_ECH_REQUIRED";
]

(* Names whose OCaml variant tag does NOT match the libcurl numeric value
   because the C side resolves them through a #define alias to a different
   code. Skip the index/value assertion for these. *)
let aliased = [
  (* renamed to CURLE_PEER_FAILED_VERIFICATION at slot 51 in libcurl 7.17.1,
     then moved to slot 60 in 7.62.0, folding in CURLE_SSL_CACERT *)
  "CURLE_SSL_PEER_CERTIFICATE";
]

let pr fmt = Printf.ksprintf print_endline fmt

let () =
  match List.tl @@ Array.to_list @@ Sys.argv with
  | [] | "c"::[] ->
    all |> List.iter begin fun s ->
      match s with
      | "CURLE_OK" ->
        pr "    {\"%s\", %s}," s s
      | _ ->
        pr "#if HAVE_DECL_%s" s;
        pr "    {\"%s\", %s}," s s;
        pr "#else";
        pr "    {\"%s\", -1}," s;
        pr"#endif";
    end
  | "c-assert"::[] ->
    all |> List.iteri begin fun i s ->
      if s <> "CURLE_OK" && not (List.mem s aliased) then begin
        pr "#if HAVE_DECL_%s" s;
        pr "_Static_assert((int)%s == %d, \"%s expected to be %d\");" s i s i;
        pr "#endif";
      end
    end
  | "ml"::[] -> all |> List.iter (pr "  | %s")
  | "configure"::[] ->
    Format.set_margin 80;
    Format.open_box 0;
    all |> List.iter (function "CURLE_OK" -> () | s -> Format.printf "%s,@ " s);
    Format.close_box ();
  | _ -> failwith "bad usage"

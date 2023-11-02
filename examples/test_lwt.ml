(** curl lwt example *)

open Printf

let (@@) f x = f x
let (|>) x f = f x

let printfn fmt = ksprintf print_endline fmt

let curl_setup_simple h =
  let open Curl in
  set_useragent h "Curl_lwt";
  set_nosignal h true;
  set_connecttimeout h 5;
  set_timeout h 10;
  set_followlocation h false;
  set_maxredirs h 1;
  set_ipresolve h IPRESOLVE_V4;
  set_encoding h CURL_ENCODING_ANY

let curl_setup_tcp h =
  let open Curl in
  set_tcpkeepalive h true;
  set_tcpkeepidle h 10;
  set_tcpkeepintvl h 10

let log_curl h code =
  let open Curl in
  let url = get_effectiveurl h in
  printfn "%3d %.2f %5.0fB URL: %s (%s)%s"
    (get_httpcode h)
    (get_totaltime h)
    (get_sizedownload h)
    (if get_httpcode h / 100 = 3 then sprintf "%s -> %s" url (get_redirecturl h) else url)
    (get_contenttype h)
    (match code with CURLE_OK -> "" | _ -> sprintf " %s (%d)" (strerror code) (errno code))

let download h =
  let b = Buffer.create 16 in
  Curl.set_writefunction h (fun s -> Buffer.add_string b s; String.length s);
  Curl.set_prereqfunction h (fun conn_primary_ip conn_local_ip conn_primary_port conn_local_port ->
    printfn "Making request %s:%d -> %s:%d"
      conn_local_ip conn_local_port conn_primary_ip conn_primary_port;
    false);
  Lwt.bind (Curl_lwt.perform h) (fun code -> Lwt.return (code, Buffer.contents b))

let get url =
  let h = Curl.init () in
  Curl.set_url h url;
  curl_setup_simple h;
  curl_setup_tcp h;
  begin try%lwt (* e.g. Canceled *)
    let%lwt (code,_body) = download h in
    log_curl h code;
    Lwt.return ()
    (* do something with body *)
  with exn ->
    printfn "EXN %s URL: %s" (Printexc.to_string exn) url;
    Lwt.fail exn
  end[%lwt.finally Curl.cleanup h; Lwt.return ()]

let urls =
  [
    "http://www.forth.org.ru";
    "http://caml.inria.fr";
    "https://www.rust-lang.org";
    "https://ocaml.org";
    "http://elm-lang.org";
    "http://www.red-lang.org";
  ]

let wait_one tasks =
  let%lwt () = Lwt_unix.sleep 0.5 in
  let%lwt () = Lwt.choose tasks in
  print_endline "Cancel remaining transfers";
  Lwt.return ()

let () =
  printfn "Launch %d transfers" (List.length urls);
  let tasks = List.map get urls in
  Lwt_main.run @@ Lwt.pick [
    wait_one tasks;
    Lwt.join tasks
  ]

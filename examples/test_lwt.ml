(** curl lwt example *)

open Printf

let (@@) f x = f x
let (|>) x f = f x

let curl_setup_simple h =
  let open Curl in
  set_useragent h "Curl_lwt";
  set_nosignal h true;
  set_connecttimeout h 5;
  set_timeout h 10;
  set_followlocation h false;
  set_maxredirs h 1;
  set_encoding h CURL_ENCODING_ANY

let log_curl h code =
  let open Curl in
  let url = get_effectiveurl h in
  print_endline @@ sprintf "%3d %.2f %g URL: %s (%s)%s"
    (get_httpcode h)
    (get_totaltime h)
    (get_sizedownload h)
    (if get_httpcode h / 100 = 3 then sprintf "%s -> %s" url (get_redirecturl h) else url)
    (get_contenttype h)
    (match code with CURLE_OK -> "" | _ -> sprintf " %s (%d)" (strerror code) (errno code))

let download h =
  let b = Buffer.create 16 in
  Curl.set_writefunction h (fun s -> Buffer.add_string b s; String.length s);
  Lwt.bind (Curl_lwt.perform h) (fun code -> Lwt.return (code, Buffer.contents b))

let get url =
  let h = Curl.init () in
  Curl.set_url h url;
  curl_setup_simple h;
(*   lwt (code,body) = download h in *)
  Lwt.bind (download h) @@ fun (code,_body) ->
  log_curl h code;
  (* do something with body *)
  Curl.cleanup h;
  Lwt.return ()

let run () =
  [
    "www.google.com";
    "ya.ru";
    "www.forth.org.ru";
    "caml.inria.fr";
    "www.mozart-oz.org";
    "forge.ocamlcore.org";
  ]
  |> List.map get
  |> Lwt.join

let () =
  Lwt_main.run @@ run ()

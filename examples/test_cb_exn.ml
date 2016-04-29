(* Copyright (c) 2013, Thomas Leonard, <talex5@gmail.com> *)

open Printf

exception Abort

let () =
  let open Curl in
  let connection = init () in
  let error_buffer = ref "" in

  set_writefunction connection (fun _ -> raise Abort);
  set_url connection "http://google.com";
  set_errorbuffer connection error_buffer;

  let run () =
    try
      perform connection;
    with
    | CurlException (CURLE_WRITE_ERROR,_,_) -> printf "ok\n%!"
    | CurlException (error,_,name) -> printf "E: wrong error: %s %s : %s\n%!" name (Curl.strerror error) !error_buffer
    | exn -> printf "E: wrong error: %s\n%!" (Printexc.to_string exn)
  in
  run ();
  run ();
  cleanup connection

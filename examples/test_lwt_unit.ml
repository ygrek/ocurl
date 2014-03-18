(* Copyright (c) 2014, Thomas Leonard, <talex5@gmail.com> *)

open Curl
open Printf

let verbose = false

let (|>) x f = f x
let printfn fmt = ksprintf print_endline fmt

let setup buf =
  let h = init () in
  set_url h "http://localhost:1/missing.png";
  set_errorbuffer h buf;
  h

let () =
  let buf1 = ref "" in
  let h = setup buf1 in
  (* easy *)
  let () = try
    perform h
  with CurlException (code,_,_) ->
    if verbose then printfn "Sync errors: %s <%s>" (strerror code) !buf1
  in
  (* lwt *)
  let buf2 = ref "" in
  let h = setup buf2 in
  let code = Curl_lwt.perform h |> Lwt_main.run in
  if verbose then printfn "Lwt errors: %s <%s>" (strerror code) !buf2;

  if buf1 <> buf2 then
    (printfn "FAILED: %S <> %S" !buf1 !buf2; exit 1)
  else
    (printfn "OK"; exit 0)

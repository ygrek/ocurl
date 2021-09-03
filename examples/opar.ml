(*
 * opar.ml
 *
 * Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 *)

open Printf

let show_progress fname conn =
  let bytes = Curl.get_sizedownload conn
  and total = Curl.get_contentlengthdownload conn in
    printf "%s : %.2f%%\n" fname (bytes /. total *. 100.0);
    flush stdout

let writer _fname _conn accum data =
  (* show_progress fname conn; *)
  Buffer.add_string accum data;
  String.length data

let save fname content =
  let fp = open_out_bin fname in
    Buffer.output_buffer fp content;
    close_out fp

let get fname url () =
  let result = Buffer.create 16384
  and conn = Curl.init () in
    Curl.set_writefunction conn (writer fname conn result);
    Curl.set_followlocation conn true;
    Curl.set_url conn url;
    Curl.perform conn;
    Curl.cleanup conn;
    save fname result

let thread_get fname url =
  Thread.create (get fname url) ()

let make_fname i =
  sprintf "file%02d" (i+1)

let rec join requests =
  match requests with
    | [] -> ()
    | (fname, t) :: rest ->
        printf "Download done [%s]\n" fname;
        Thread.join t;
        join rest

let _ =
  Curl.global_init Curl.CURLINIT_GLOBALALL;
  let requests = Array.mapi
    (fun i url ->
       let fname = make_fname i in
         (fname, thread_get fname url))
    (Array.sub Sys.argv 1 (Array.length Sys.argv - 1))
  in
    join (Array.to_list requests)

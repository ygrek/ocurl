(*
 * omulti.ml
 *
 * Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 *)

module A = Array
module M = Curl.Multi

let pr fmt = Printf.ksprintf print_endline fmt

let finished mt =
  let rec loop n =
  match M.remove_finished mt with
  | None -> (*if n > 0 then pr "Removed %u handles" n*) ()
  | Some _ -> loop (n+1)
  in loop 0

let loop_wait hs mt =
  pr "perform/wait";
  while M.perform mt > 0 do
    ignore (M.wait mt)
  done;
  finished mt

let mutexify m fn v =
  Mutex.lock m;
  try
    let ret = fn v in
    Mutex.unlock m;
    ret
  with exn ->
    Mutex.unlock m;
    raise exn

let loop_select hs mt =
  pr "action/select";
  let in_fd = ref [] in
  let out_fd = ref [] in
  let is_done = ref (A.length hs = 0) in
  let m = Mutex.create () in
  let on_event fd event =
    if M.action mt fd event = 0 then
      mutexify m (fun () ->
        is_done := true
      ) ();
  in
  M.set_socket_function mt (fun fd what -> 
    A.iter (fun (h, url) ->
      if Some fd = Curl.get_activesocket h then
        pr "Waiting for event on socket for url %s" url) hs;
    mutexify m (fun () ->
      in_fd := List.filter ((<>) fd) !in_fd;
      out_fd := List.filter ((<>) fd) !out_fd;
      (match what with
         | M.POLL_REMOVE | M.POLL_NONE -> ()
         | M.POLL_IN -> in_fd := fd :: !in_fd
         | M.POLL_OUT -> out_fd := fd :: !out_fd
         | M.POLL_INOUT -> in_fd := fd :: !in_fd; out_fd := fd :: !out_fd)) ()
  );
  let rec process () =
    M.action_timeout mt;
    let is_done = 
      mutexify m (fun () ->
        !is_done
      ) ()
    in
    if not is_done then (
      let timeout =
        if !in_fd = [] && !out_fd = [] then 0.1 else -1.
      in
      let (fdi,fdo,_) = Unix.select !in_fd !out_fd [] timeout in
      List.iter (fun fd -> on_event fd M.EV_IN) fdi;
      List.iter (fun fd -> on_event fd M.EV_OUT) fdo;
      process ()
    )
  in
  let th = Thread.create process () in
  Thread.join th;
  finished mt

let input_lines file =
  let ch = open_in file in
  let lines = ref [] in
  try while true do lines := input_line ch :: !lines done; []
  with End_of_file -> close_in_noerr ch; List.rev !lines

let () =
  let func = ref None in
  let urls = 
    let urls = ref [] in
    let n = ref 10 in
    let args = Arg.align 
      ["-n",Arg.Set_int n,"<N> ";
       "-i",Arg.String (fun s -> urls := input_lines s @ !urls),"<file> read urls from file";
       "-l",Arg.String (fun s -> urls := s :: !urls),"<url> fetch url";
       "-m",Arg.String (function 
           | "wait" -> func := Some loop_wait
           | "select" -> func := Some loop_select
           | s-> failwith (Printf.sprintf "unknown method : %s" s)), "<wait|select> loop method";
      ]
    in
    Arg.parse args failwith "Options :";
    match !urls with
    | [url] -> A.make !n url
    | l -> A.of_list l
  in
(*   if A.length urls = 0 then failwith "Specify urls to download"; *)
  let init url =
    let h = Curl.init () in
    Curl.set_url h url;
    Curl.set_writefunction h String.length;
    (h, url)
  in
  let cleanup (h, _) =
    Printf.printf "Time: %f Size: %Lu Speed: %.2f KB/s URL: %s\n"
      (Curl.get_totaltime h)
      (Int64.of_float (Curl.get_sizedownload h))
      (Curl.get_speeddownload h /. 1024.)
      (Curl.get_effectiveurl h);
    Curl.cleanup h
  in
  let test loop =
   let hs = A.map init urls in
   let mt = M.create () in
   A.iter (fun (h, _) -> M.add mt h) hs;
   loop hs mt;
   A.iter cleanup hs;
   M.cleanup mt;
   pr "Finished";
  in
  match !func with
  | None -> test loop_wait; test loop_select
  | Some f -> test f


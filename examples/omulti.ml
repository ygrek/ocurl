(*
 * omulti.ml
 *
 * Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 *)

module M = Curl.Multi
module Ev = Liboevent

let pr fmt = Printf.ksprintf print_endline fmt

let finished mt =
  let rec loop n =
  match M.remove_finished mt with
  | None -> (*if n > 0 then pr "Removed %u handles" n*) ()
  | Some _ -> loop (n+1)
  in loop 0

let loop_wait mt =
  pr "perform/wait";
  while M.perform mt > 0 do
    ignore (M.wait mt)
  done;
  finished mt

let events_base = Ev.init ()

let loop_async mt =
  pr "action/event";
  let events = Hashtbl.create 32 in
  let on_event fd flags =
    let event = match flags with
                | Ev.READ -> M.EV_IN
                | Ev.WRITE -> M.EV_OUT
                | _ -> M.EV_AUTO
    in
    let _ = M.action mt fd event in
    finished mt
  in
  let evs = ref 0 in
  M.set_socket_f mt begin fun h fd what ->
    List.iter (fun ev -> decr evs; Ev.del ev) (Hashtbl.find_all events fd); Hashtbl.remove events fd;
    let flags = match what with
          | M.POLL_REMOVE | M.POLL_NONE -> []
          | M.POLL_IN -> [Ev.READ]
          | M.POLL_OUT -> [Ev.WRITE]
          | M.POLL_INOUT -> [Ev.READ;Ev.WRITE]
    in
    match flags with
    | [] -> finished mt
    | flags ->
      let ev = Ev.create () in
      Ev.set ev fd flags true on_event;
      Ev.add events_base ev None;
      incr evs;
      Hashtbl.add events fd ev
  end;
  let _ = M.action_all mt in
  Ev.dispatch events_base;
  assert (0 = !evs);
  finished mt

let loop_select mt =
  pr "action/select";
  let in_fd = ref [] in
  let out_fd = ref [] in
  let on_event fd event =
    let _ = M.action mt fd event in
    finished mt
  in
  M.set_socket_f mt begin fun h fd what ->
    in_fd := List.filter ((<>) fd) !in_fd;
    out_fd := List.filter ((<>) fd) !out_fd;
    match what with
    | M.POLL_REMOVE | M.POLL_NONE -> finished mt
    | M.POLL_IN -> in_fd := fd :: !in_fd
    | M.POLL_OUT -> out_fd := fd :: !out_fd
    | M.POLL_INOUT -> in_fd := fd :: !in_fd; out_fd := fd :: !out_fd
  end;
  let _ = M.action_all mt in
  while !in_fd <> [] || !out_fd <> [] do
    let (fdi,fdo,_) = Unix.select !in_fd !out_fd [] (-1.) in
    List.iter (fun fd -> on_event fd M.EV_IN) fdi;
    List.iter (fun fd -> on_event fd M.EV_OUT) fdo;
  done;
  finished mt

let input_lines file =
  let ch = open_in file in
  let lines = ref [] in
  try while true do lines := input_line ch :: !lines done; []
  with End_of_file -> close_in_noerr ch; List.rev !lines

let () =
  let module A = Array in
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
           | "event" -> func := Some loop_async
           | "select" -> func := Some loop_select
           | s-> failwith (Printf.sprintf "unknown method : %s" s)), "<wait|event|select> loop method";
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
    h
  in
  let cleanup h =
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
   A.iter (M.add mt) hs;
   loop mt;
   A.iter cleanup hs;
   M.cleanup mt;
   pr "Finished";
  in
  match !func with
  | None -> test loop_wait; test loop_select; test loop_async
  | Some f -> test f


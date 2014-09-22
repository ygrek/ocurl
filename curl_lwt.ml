(** Lwt support for Curl *)

module M = Curl.Multi

let debug = ref false
let log fmt = Printf.ksprintf prerr_endline fmt

let set_debug x = debug := x

let int_of_fd : Unix.file_descr -> int = Obj.magic

type multi = {
  mt : Curl.Multi.mt;
  all_events : (Unix.file_descr, Lwt_engine.event list) Hashtbl.t;
  wakeners : (Curl.t, Curl.curlCode Lwt.u) Hashtbl.t;
}

let create () =
  let mt = M.create () in
  let timer_event = ref Lwt_engine.fake_event in
  let all_events = Hashtbl.create 32 in
  let wakeners = Hashtbl.create 32 in
  let finished s =
    if !debug then log "finished %s" s;
    let rec loop n =
      match M.remove_finished mt with
      | None -> if n > 0 && !debug then log "removed %u handles via %s" n s
      | Some (h,code) ->
        if !debug then log "wakeup";
        begin try
          let w = Hashtbl.find wakeners h in
          Hashtbl.remove wakeners h;
          Lwt.wakeup w code
        with Not_found ->
          prerr_endline "curl_lwt: orphan handle, how come?"
        end;
        loop (n+1)
    in
    loop 0
  in
  let on_readable fd _ =
    if !debug then log "on_readable fd %d" (int_of_fd fd);
    let (_:int) = M.action mt fd M.EV_IN in
    finished "on_readable";
  in
  let on_writable fd _ =
    if !debug then log "on_writable fd %d" (int_of_fd fd);
    let (_:int) = M.action mt fd M.EV_OUT in
    finished "on_writable";
  in
  let on_timer _ =
    if !debug then log "on_timer";
    Lwt_engine.stop_event !timer_event;
    M.action_timeout mt;
    finished "on_timer"
  in
  M.set_timer_function mt begin fun timeout ->
    if !debug then log "set timeout %d" timeout;
    Lwt_engine.stop_event !timer_event; (* duplicate stop_event is ok *)
    timer_event := Lwt_engine.on_timer (float_of_int timeout /. 1000.) false on_timer
  end;
  M.set_socket_function mt begin fun fd what ->
    if !debug then log "set socket fd %d %s" (int_of_fd fd)
      (match what with
      | M.POLL_NONE -> "none"
      | M.POLL_REMOVE -> "remove"
      | M.POLL_IN -> "in"
      | M.POLL_OUT -> "out"
      | M.POLL_INOUT -> "inout");
    begin
    try
      List.iter Lwt_engine.stop_event (Hashtbl.find all_events fd);
      Hashtbl.remove all_events fd;
      if !debug then log "removed handlers for %d" (int_of_fd fd);
    with
      Not_found -> () (* first event for the socket - no association *)
    end;
    let events = match what with
    | M.POLL_REMOVE | M.POLL_NONE -> []
    | M.POLL_IN -> [Lwt_engine.on_readable fd (on_readable fd)]
    | M.POLL_OUT -> [Lwt_engine.on_writable fd (on_writable fd)]
    | M.POLL_INOUT -> [Lwt_engine.on_readable fd (on_readable fd); Lwt_engine.on_writable fd (on_writable fd)]
    in
    match events with
    | [] -> ()
    | _ -> Hashtbl.add all_events fd events;
  end;
  { mt; all_events; wakeners; }

(* lwt may not run in parallel so one global is OK'ish *)
let global = lazy (create ())

let perform h =
  let t = Lazy.force global in
  let (waiter,wakener) = Lwt.wait () in
  let waiter = Lwt.protected waiter in
  Lwt.on_cancel waiter (fun () ->
    Curl.Multi.remove t.mt h;
    Hashtbl.remove t.wakeners h;
  );
  Hashtbl.add t.wakeners h wakener;
  M.add t.mt h;
  waiter

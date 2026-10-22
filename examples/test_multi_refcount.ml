(* Lifetime of easy handles added to a multi handle *)

module M = Curl.Multi

(* runs the transfers until h is finished and removed *)
let run mt h =
  let rec loop () =
    ignore (M.perform mt);
    match M.remove_finished mt with
    | Some (h', _) -> assert (h' = h)
    | None -> ignore (M.wait ~timeout_ms:100 mt); loop ()
  in
  loop ()

let finish mt h =
  M.add mt h;
  run mt h

(* Multi.remove after Multi.remove_finished must not release the multi's
   reference to h a second time, which used to free it while still in use.
   https://github.com/ygrek/ocurl/pull/113 *)
let test_remove_after_finished () =
  let mt = M.create () in
  let h = Curl.init () in
  for _ = 1 to 100 do
    (* no URL set, so the transfer fails immediately without touching the network *)
    finish mt h;
    (* already removed: a no-op, or an error before libcurl 7.33.0 *)
    (try M.remove mt h with M.CError _ -> ());
    (* collect the second OCaml block returned by remove_finished *)
    Gc.full_major ();
    (* h must still refer to a live connection *)
    Curl.reset h
  done;
  Curl.cleanup h;
  M.cleanup mt

(* adds a handle no OCaml value refers to, with w tracking its private data,
   which lives as long as the connection *)
let[@inline never] add_unreferenced mt w =
  let h = Curl.init () in
  let s = String.make 16 'x' in
  Curl.set_private h s;
  Weak.set w 0 (Some s);
  M.add mt h

(* The multi keeps added handles alive until it is cleaned up *)
let test_multi_cleanup () =
  let w = Weak.create 1 in
  let mt = M.create () in
  add_unreferenced mt w;
  Gc.full_major ();
  assert (Weak.check w 0);
  M.cleanup mt;
  Gc.full_major ();
  assert (not (Weak.check w 0));
  (* handles still referenced from OCaml stay usable *)
  let mt = M.create () in
  let h = Curl.init () in
  M.add mt h;
  M.cleanup mt;
  Gc.full_major ();
  Curl.reset h;
  Curl.cleanup h

(* Curl.cleanup also removes the handle from the multi *)
let test_easy_cleanup () =
  let mt = M.create () in
  let h = Curl.init () in
  M.add mt h;
  Curl.cleanup h;
  Gc.full_major ();
  M.cleanup mt

(* a refused connection opens and closes a socket without needing a server *)
let set_refused h =
  Curl.set_url h "http://127.0.0.1:1/";
  Curl.set_timeoutms h 5000

let perform_refused h =
  set_refused h;
  try Curl.perform h with Curl.CurlException _ -> ()

(* The multi's closesocket function is only used while the handle is added *)
let test_closesocket () =
  let closed = ref 0 in
  let mt = M.create () in
  M.set_closesocket_function mt (fun fd -> incr closed; Unix.close fd);
  let removed = Curl.init () in
  set_refused removed;
  finish mt removed;
  assert (!closed > 0);
  closed := 0;
  (* not once removed *)
  perform_refused removed;
  assert (!closed = 0);
  (* nor after failing to add a handle that is in another multi handle *)
  let other = M.create () in
  let h = Curl.init () in
  set_refused h;
  M.add other h;
  (try M.add mt h; assert false with M.CError _ -> ());
  run other h;
  assert (!closed = 0);
  M.cleanup other;
  Curl.cleanup h;
  (* nor after Multi.cleanup freed the multi *)
  let added = Curl.init () in
  M.add mt added;
  M.cleanup mt;
  perform_refused added;
  perform_refused removed;
  assert (!closed = 0);
  Curl.cleanup added;
  Curl.cleanup removed

let () =
  test_remove_after_finished ();
  test_multi_cleanup ();
  test_easy_cleanup ();
  test_closesocket ();
  print_endline "OK"

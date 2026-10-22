(* Lifetime of easy handles added to a multi handle *)

module M = Curl.Multi

(* no URL set, so the transfer fails immediately without touching the network *)
let finish mt h =
  M.add mt h;
  let rec loop () =
    ignore (M.perform mt);
    match M.remove_finished mt with
    | Some (h', _) -> assert (h' = h)
    | None -> ignore (M.wait ~timeout_ms:100 mt); loop ()
  in
  loop ()

(* Multi.remove after Multi.remove_finished must not release the multi's
   reference to h a second time, which used to free it while still in use.
   https://github.com/ygrek/ocurl/pull/113 *)
let test_remove_after_finished () =
  let mt = M.create () in
  let h = Curl.init () in
  for _ = 1 to 100 do
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

let () =
  test_remove_after_finished ();
  test_multi_cleanup ();
  test_easy_cleanup ();
  print_endline "OK"

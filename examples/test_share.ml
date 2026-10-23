(* Share handles *)

module S = Curl.Share
module M = Curl.Multi

let share data =
  let s = S.init () in
  List.iter (fun d -> S.setopt s (S.CURLSHOPT_SHARE d)) data;
  s

let easy s =
  let h = Curl.init () in
  Curl.set_share h s;
  h

let share_error code f =
  match f () with
  | () -> assert false
  | exception S.ShareError (code', _, _) -> assert (code' = code)

let invalid_argument f =
  match f () with
  | () -> assert false
  | exception Invalid_argument _ -> ()

(* in netscape format *)
let set_cookie h name =
  Curl.set_cookielist h (Printf.sprintf "example.com\tFALSE\t/\tFALSE\t0\t%s\t1" name)

let cookies h = List.length (Curl.get_cookielist h)

let test_cookies () =
  let s = share [S.CURLSHOPT_SHARE_COOKIE] in
  let h1 = easy s in
  let h2 = easy s in
  let other = Curl.init () in
  set_cookie h1 "a";
  set_cookie other "b";
  assert (cookies h2 = 1);
  assert (cookies other = 1);
  (* reset keeps the share handle *)
  Curl.reset h2;
  assert (cookies h2 = 1);
  List.iter Curl.cleanup [h1; h2; other];
  S.cleanup s

let test_cleanup () =
  let s = share [S.CURLSHOPT_SHARE_DNS] in
  let h = easy s in
  share_error S.CURLSHE_IN_USE (fun () -> S.cleanup s);
  share_error S.CURLSHE_IN_USE (fun () -> S.setopt s (S.CURLSHOPT_SHARE S.CURLSHOPT_SHARE_COOKIE));
  Curl.cleanup h;
  S.cleanup s;
  S.cleanup s;
  share_error S.CURLSHE_INVALID (fun () -> S.setopt s (S.CURLSHOPT_SHARE S.CURLSHOPT_SHARE_COOKIE));
  let h = Curl.init () in
  share_error S.CURLSHE_INVALID (fun () -> Curl.set_share h s);
  Curl.cleanup h;
  assert (S.strerror S.CURLSHE_IN_USE <> "")

(* sets a share handle no OCaml value refers to, with w tracking it *)
let[@inline never] set_unreferenced h w =
  let s = share [S.CURLSHOPT_SHARE_COOKIE] in
  Weak.set w 0 (Some s);
  Curl.set_share h s

(* easy handles keep their share handle alive *)
let test_lifetime () =
  let w = Weak.create 1 in
  let h = Curl.init () in
  set_unreferenced h w;
  Gc.full_major ();
  assert (Weak.check w 0);
  Curl.reset h;
  Gc.full_major ();
  assert (Weak.check w 0);
  set_cookie h "a";
  assert (cookies h = 1);
  let s = Option.get (Weak.get w 0) in
  Curl.cleanup h;
  S.cleanup s

(* connections shared through a share handle could be closed with the multi's
   closesocket function after the multi handle is freed *)
let test_closesocket () =
  let connections = share [S.CURLSHOPT_SHARE_CONNECT] in
  let dns = share [S.CURLSHOPT_SHARE_DNS] in
  let closesocket = M.create () in
  M.set_closesocket_function closesocket Unix.close;
  let h = easy connections in
  invalid_argument (fun () -> M.add closesocket h);
  (* fine without a closesocket function *)
  let mt = M.create () in
  M.add mt h;
  invalid_argument (fun () -> M.set_closesocket_function mt Unix.close);
  M.remove mt h;
  M.set_closesocket_function mt Unix.close;
  (* fine without sharing connections *)
  Curl.set_share h dns;
  M.add closesocket h;
  invalid_argument (fun () -> Curl.set_share h connections);
  M.remove closesocket h;
  Curl.set_share h connections;
  Curl.cleanup h;
  M.cleanup mt;
  M.cleanup closesocket;
  S.cleanup connections;
  S.cleanup dns

let () =
  test_cookies ();
  test_cleanup ();
  test_lifetime ();
  test_closesocket ();
  print_endline "OK"

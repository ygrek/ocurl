(* CURLOPT_SOCKOPTFUNCTION: what the callback is called with, and what libcurl
   does when it succeeds, fails or raises. Needs no network, connects to a
   local listener. *)

open Printf

let failed = ref false

let check name cond =
  if not cond then (failed := true; printf "E: %s\n%!" name)

let is_connected fd =
  match Unix.getpeername fd with
  | _ -> true
  | exception Unix.Unix_error (Unix.ENOTCONN, _, _) -> false

(* The listener never accepts, the kernel completes connections in its backlog. *)
let with_listener f =
  let listener = Unix.socket Unix.PF_INET Unix.SOCK_STREAM 0 in
  Unix.bind listener (Unix.ADDR_INET (Unix.inet_addr_loopback, 0));
  Unix.listen listener 10;
  let port =
    match Unix.getsockname listener with
    | Unix.ADDR_INET (_, port) -> port
    | Unix.ADDR_UNIX _ -> assert false
  in
  Fun.protect ~finally:(fun () -> Unix.close listener)
    (fun () -> f (sprintf "http://127.0.0.1:%d/" port))

(* Runs a CONNECT_ONLY transfer with [callback] installed. Returns what the
   callback was called with and whether the transfer's socket is connected,
   None if the transfer failed. *)
let connect url callback =
  let open Curl in
  let h = init () in
  set_url h url;
  set_connectonly h true;
  let calls = ref [] in
  set_sockoptfunction h (fun fd purpose ->
    calls := (fd, purpose, is_connected fd) :: !calls;
    callback ());
  let result =
    match perform h with
    | () ->
      (match get_activesocket h with
       | Some fd -> Some (fd, is_connected fd)
       | None -> failwith "no active socket")
    | exception CurlException _ -> None
  in
  cleanup h;
  List.rev !calls, result

let test name url callback ~expect_ok =
  let calls, result = connect url callback in
  (match calls with
   | [ (_, purpose, connected) ] ->
     check (name ^ ": purpose") (purpose = Curl.CURLSOCKTYPE_IPCXN);
     check (name ^ ": called before connect") (not connected)
   | _ -> check (sprintf "%s: called %d times" name (List.length calls)) false);
  match result, calls with
  | Some (fd, connected), [ (fd', _, _) ] ->
    check (name ^ ": transfer should fail") expect_ok;
    check (name ^ ": socket is the one passed to callback") (fd = fd');
    check (name ^ ": connected") connected
  | Some _, _ -> ()
  | None, _ -> check (name ^ ": transfer should succeed") (not expect_ok)

exception Abort

let () =
  with_listener begin fun url ->
    test "ok" url (fun () -> Curl.CURL_SOCKOPT_OK) ~expect_ok:true;
    test "error" url (fun () -> Curl.CURL_SOCKOPT_ERROR) ~expect_ok:false;
    test "exception" url (fun () -> raise Abort) ~expect_ok:false
  end;
  if !failed then exit 1 else printf "ok\n%!"

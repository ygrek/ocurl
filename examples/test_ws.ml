open Curl

let received_data = ref ""
let received_mutex = Mutex.create ()
let received_intro = ref false

let write_callback conn data =
  Mutex.lock received_mutex;
  if not !received_intro then (
    received_intro := true;
    Printf.printf "Received intro: %s\n%!" data;
  ) else begin
    match ws_meta conn with
    | Some frame ->
      if List.mem CURLWS_TEXT frame.flags then (
        received_data := !received_data ^ data;
        Printf.printf "Received TEXT: %s\n%!" data;
      ) else (
        Printf.printf "Received non-TEXT frame: ";
        List.iter (function
          | CURLWS_TEXT -> Printf.printf "TEXT;"
          | CURLWS_BINARY -> Printf.printf "BINARY;"
          | CURLWS_CONT -> Printf.printf "CONT;"
          | CURLWS_CLOSE -> Printf.printf "CLOSE;"
          | CURLWS_PING -> Printf.printf "PING;"
          | CURLWS_PONG -> Printf.printf "PONG;"
          | CURLWS_OFFSET -> Printf.printf "OFFSET;"
        ) frame.flags;
        Printf.printf "\n%!";
      )
    | None ->
      Printf.printf "Received data without frame info: %s\n%!" data;
      received_data := !received_data ^ data
   end;
  Mutex.unlock received_mutex;
  String.length data

let () =
  global_init CURLINIT_GLOBALALL;
  let conn = init () in
  (try
    setopt conn (CURLOPT_URL "wss://echo.websocket.org/");
    setopt conn (CURLOPT_WRITEFUNCTION (write_callback conn));
    setopt conn (CURLOPT_VERBOSE false);
    Printf.printf "Starting WebSocket connection...\n%!";
    let _ = Thread.create (fun () ->
      try
        perform conn
      with
      | e -> Printf.eprintf "Connection error: %s\n%!" (Printexc.to_string e)
    ) () in
    Unix.sleep 5;
    let test_message = "Hello websocket from OCaml" in
    (try
      let bytes_sent = ws_send conn test_message [CURLWS_TEXT] in
      Printf.printf "Sent %d bytes: '%s'\n%!" bytes_sent test_message;
    with
    | e -> Printf.eprintf "Send error: %s\n%!" (Printexc.to_string e));
    Unix.sleep 5;
    Mutex.lock received_mutex;
    Printf.printf "Final received data: '%s'\n%!" !received_data;
    received_data := "";
    Mutex.unlock received_mutex;
    Unix.sleep 5;
    (try
      let bytes_sent = ws_send conn "ping" [CURLWS_PING] in
      Printf.printf "Sent %d bytes: ping\n%!" bytes_sent;
    with
    | e -> Printf.eprintf "Send error: %s\n%!" (Printexc.to_string e));
    Unix.sleep 5;
    Printf.printf "WebSocket echo test completed!\n%!";
  with
  | CurlException (code, _, msg) ->
    Printf.eprintf "Curl error %s: %s\n%!" (strerror code) msg
  | e ->
    Printf.eprintf "Other error: %s\n%!" (Printexc.to_string e));
  cleanup conn;
  global_cleanup ()
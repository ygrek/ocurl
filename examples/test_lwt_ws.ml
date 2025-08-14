open Curl

let write_callback conn data =
  begin
    match ws_meta conn with
    | Some frame ->
      if List.mem CURLWS_TEXT frame.flags then (
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
      Printf.printf "Received data without frame info: %s\n%!" data
    end;
  String.length data

let () =
  let conn = init () in
  let send_message () =
    Lwt_unix.sleep 5.0;%lwt
    let test_message = "Hello websocket from OCaml" in
    (try
      let bytes_sent = ws_send conn test_message [CURLWS_TEXT] in
      Lwt_io.printf "Sent %d bytes: '%s'\n%!" bytes_sent test_message
    with
    | e -> Lwt_io.eprintf "Send error: %s\n%!" (Printexc.to_string e))
  in
  let send_ping () =
    Lwt_unix.sleep 5.0;%lwt
    (try
      let bytes_sent = ws_send conn "ping" [CURLWS_PING] in
      Lwt_io.printf "Sent %d bytes: ping\n%!" bytes_sent
    with
    | e -> Lwt_io.eprintf "Send error: %s\n%!" (Printexc.to_string e))
  in
  Lwt_main.run begin
  (try
    setopt conn (CURLOPT_URL "wss://echo.websocket.org/");
    setopt conn (CURLOPT_WRITEFUNCTION (write_callback conn));
    setopt conn (CURLOPT_VERBOSE false);
    Lwt_io.printf "Starting WebSocket connection...\n%!";%lwt
    let perform () =
      let%lwt code = Curl_lwt.perform conn in
      Lwt_io.printf "WebSocket echo test completed with code: %d\n%!" (Curl.int_of_curlCode code)
    in
    let wait_and_stop () =
      Lwt_unix.sleep 10.0;%lwt
      Lwt_io.printf "Stopping WebSocket connection...\n%!"
    in
    Lwt.choose [
      wait_and_stop ();
      Lwt.join [perform (); send_message (); send_ping ()]
    ]
  with
  | CurlException (code, _, msg) ->
    Lwt_io.eprintf "Curl error %s: %s\n%!" (strerror code) msg
  | e ->
    Lwt_io.eprintf "Other error: %s\n%!" (Printexc.to_string e));%lwt
  cleanup conn;
  Lwt.return_unit
  end
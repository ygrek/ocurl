open Curl

let count1 = 200
let leak1 = 1024 * 1024

let test1 () =
  let h = init () in
  let s = String.make leak1 'c' in
  set_httppost h [CURLFORM_CONTENT ("part", s, DEFAULT)];
  cleanup h

let rss () =
  let path = Printf.sprintf "/proc/%d/statm" (Unix.getpid ()) in
  match open_in path with
  | exception exn -> Printf.eprintf "Error opening %s (%s), ignoring\n%!" path (Printexc.to_string exn); 0
  | ch -> let n = Scanf.fscanf ch "%_d %d" (fun x -> 4*1024*x) in close_in_noerr ch; n

let () =
  let rss1 = rss () in
  for i = 0 to count1 do
    test1 ();
    Gc.compact ();
  done;
  let rss2 = rss () in
  Printf.printf "RSS %d -> %d %s\n%!" rss1 rss2 (if rss2 - rss1 < count1 * leak1 / 10 then "OK" else "LEAKING")

(*
 * omulti.ml
 *
 * Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 *)

module M = Curl.Multi

let loop mt =
  while M.perform mt > 0 do
    ignore (M.wait mt)
  done;
  let rec rm n =
    match M.remove_finished mt with
    | None -> n
    | Some _ -> rm (n+1)
  in
  let n = rm 0 in
  Printf.printf "Removed %u handles\n" n

let () =
  let n = try int_of_string (Sys.argv.(1)) with _ -> 10 in
  let module C = Array in
  let urls = C.make n "http://localhost:8080/files/test.tar.gz" in
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
  let hs = C.map init urls in
  let mt = M.create () in
  C.iter (M.add mt) hs;
  loop mt;
  C.iter cleanup hs;
  M.cleanup mt;
  ()


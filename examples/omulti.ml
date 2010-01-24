(*
 * omulti.ml
 *
 * Copyright (c) 2009, ygrek, <ygrek@autistici.org>
 *)

module M = Curl.Multi

let loop mt =
  (*
  while M.perform mt > 0 do
    ignore (M.wait mt)
  done;
  *)
  M.events mt;
  let rec rm n =
    match M.remove_finished mt with
    | None -> n
    | Some _ -> rm (n+1)
  in
  let n = rm 0 in
  Printf.printf "Removed %u handles\n" n

let input_lines file =
  let ch = open_in file in
  let lines = ref [] in
  try while true do lines := input_line ch :: !lines done; []
  with End_of_file -> close_in_noerr ch; List.rev !lines

let () =
  let module A = Array in
  let urls = 
    let urls = ref [] in
    let n = ref 10 in
    let args = Arg.align 
      ["-n",Arg.Set_int n,"<N> ";
       "-i",Arg.String (fun s -> urls := input_lines s @ !urls),"<file> read urls from file";
       "-l",Arg.String (fun s -> urls := s :: !urls),"<url> fetch url";
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
  let hs = A.map init urls in
  let mt = M.create () in
  A.iter (M.add mt) hs;
  loop mt;
  A.iter cleanup hs;
  M.cleanup mt;
  print_endline "Finished";
  ()


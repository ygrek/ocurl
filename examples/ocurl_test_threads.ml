(** 
  Test threads and GC interaction in bindings 

  ocamlopt -thread -c -I .. ocurl_test_threads.ml -o ocurl_test_threads.cmx
  ocamlopt -thread unix.cmxa threads.cmxa -I .. -ccopt -L.. curl.cmxa ocurl_test_threads.cmx -o ocurl_test_threads
*)

let read_file connection path =
  let id = (Thread.id (Thread.self ())) in
  Printf.printf "%u read_file %s...\n%!" id path;
  let received_size = ref 0L in
  let writefunction buffer =
    let buffer_size = String.length buffer in
    Printf.printf "W(%d)%!" buffer_size;
    received_size := Int64.add !received_size (Int64.of_int buffer_size);
    buffer_size
  in
  Curl.set_httpget connection true;
  Curl.set_url connection path;
  Curl.set_writefunction connection writefunction;
  Curl.perform connection;
  Printf.printf "%u done \n%!" id;
  match Curl.get_responsecode connection with
  | 200 ->
    !received_size
  | _ as code ->
    failwith (Printf.sprintf "read_file %d" code)

let () =
  Curl.global_init Curl.CURLINIT_GLOBALALL;
  let f () = 
    let connection = Curl.init () in
    let path = if Array.length Sys.argv > 1 then Sys.argv.(1) else "http://localhost:8080/files/test.tar.gz" in
    while true do
      ignore (read_file connection path)
    done
  in
  let a = Array.init 20 (fun _ -> Thread.create f ()) in
  Array.iter Thread.join a


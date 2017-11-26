(*
 * oput.ml
 *
 * Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 *)

let counter = ref 0

let reader file maxBytes =
  let buffer = Bytes.create maxBytes in
  let readBytes = input file buffer 0 maxBytes in
  if readBytes = 0 then
    ""
  else
  begin
    counter := !counter + readBytes;
	  Bytes.sub_string buffer 0 readBytes
  end

let _ =
  if Array.length Sys.argv = 3 then
    begin
      Curl.global_init Curl.CURLINIT_GLOBALNOTHING;
      begin
	let conn = Curl.init ()
	and file = Sys.argv.(1)
	and location = Sys.argv.(2) in
	let fileContent = open_in file in
	  Curl.set_upload conn true;
	  Curl.set_url conn location;
	  Curl.set_readfunction conn (reader fileContent);
	  Curl.perform conn;
	  Curl.cleanup conn;
	  Printf.printf "Uploaded %d bytes\n" !counter
      end;
      Curl.global_cleanup ()
    end
  else
    Printf.printf "Usage: oput <ftp location> <file>\n"

(*
 * ocurl.ml
 *
 * Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaleon.com>
 *)


let writer accum data =
  Buffer.add_string accum data;
  String.length data

let showContent content =
  Printf.printf "%s" (Buffer.contents content);
  flush stdout

let showInfo connection =
  Printf.printf "Time: %f\nURL: %s\n"
    (Curl.get_totaltime connection)
    (Curl.get_effectiveurl connection)

let getContent connection url =
  Curl.set_url connection url;
  Curl.perform connection

let _ =
  Curl.global_init Curl.CURLINIT_GLOBALALL;
  begin
    let result = Buffer.create 16384
    and errorBuffer = ref "" in
    try
      let connection = Curl.init () in
	Curl.set_errorbuffer connection errorBuffer;
	Curl.set_writefunction connection (writer result);
	Curl.set_followlocation connection true;
	Curl.set_url connection Sys.argv.(1);
	Curl.perform connection;
	showContent result;
	showInfo connection;
	Curl.cleanup connection
    with
      | Curl.CurlException (reason, code, str) ->
	  Printf.fprintf stderr "Error: %s\n" !errorBuffer
      | Failure s ->
	  Printf.fprintf stderr "Caught exception: %s\n" s
  end;
  Curl.global_cleanup ()

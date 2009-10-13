(*
 * ossl.ml
 *
 * Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 *)

let _ =
  if (Array.length Sys.argv) = 2 then
    begin
      Curl.global_init Curl.CURLINIT_GLOBALALL;
      let connection = Curl.init () in
	Curl.set_url connection Sys.argv.(1);
	Curl.set_sslverifypeer connection false;
	Curl.set_sslverifyhost connection Curl.SSLVERIFYHOST_EXISTENCE;
	Curl.perform connection
    end
  else
    Printf.fprintf stderr "Usage: %s <url>\n" Sys.argv.(0)

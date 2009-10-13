(*
 * ominimal.ml
 *
 * Copyright (c) 2003-2008, Lars Nilsson, <lars@quantumchamaeleon.com>
 *)

let _ =
  if (Array.length Sys.argv) = 2 then
    begin
      Curl.global_init Curl.CURLINIT_GLOBALALL;
      let connection = new Curl.handle in
	connection#set_url Sys.argv.(1);
	connection#perform;
    end
  else
    Printf.fprintf stderr "Usage: %s <url>\n" Sys.argv.(0)

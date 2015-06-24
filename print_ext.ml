match List.tl (Array.to_list Sys.argv) with
| ["dll"] -> print_endline Config.ext_dll
| ["obj"] -> print_endline Config.ext_obj
| ["lib"] -> print_endline Config.ext_lib
| _ -> prerr_endline "print_ext.ml: wrong usage"; exit 2

open Printf

let printfn fmt = ksprintf print_endline fmt

external outdated_enums : unit -> string list = "caml_curl_outdated_enums"

let () =
  match outdated_enums () with
  | [] -> exit 0
  | l ->
    print_endline "need update for :";
    List.iter print_endline l;
    exit 1

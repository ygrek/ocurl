open Printf

let printfn fmt = ksprintf print_endline fmt

external check_enums : unit -> (int * int * string) array = "caml_curl_check_enums"

let () =
  let ok = ref true in
  Array.iter begin fun (last_used,last,name) ->
    if last_used + 1 <> last then
    begin
      ok := false;
      printfn "Need update for %s" name
    end
  end (check_enums ());
  exit (if !ok then 0 else 1)

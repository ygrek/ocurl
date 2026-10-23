(* Easy handles using the same share handle in different threads *)

module S = Curl.Share

let () =
  let s = S.init () in
  List.iter (fun d -> S.setopt s (S.CURLSHOPT_SHARE d))
    [S.CURLSHOPT_SHARE_COOKIE; S.CURLSHOPT_SHARE_DNS; S.CURLSHOPT_SHARE_SSL_SESSION];
  (* refused connections perform transfers using the shared data without needing a server *)
  let run i =
    let h = Curl.init () in
    Curl.set_share h s;
    Curl.set_url h "http://localhost:1/";
    Curl.set_timeoutms h 5000;
    for j = 1 to 50 do
      Curl.set_cookielist h (Printf.sprintf "localhost\tFALSE\t/\tFALSE\t0\tc%d_%d\t1" i j);
      try Curl.perform h with Curl.CurlException _ -> ()
    done;
    Curl.cleanup h
  in
  List.init 4 (Thread.create run) |> List.iter Thread.join;
  let h = Curl.init () in
  Curl.set_share h s;
  assert (List.length (Curl.get_cookielist h) = 4 * 50);
  Curl.cleanup h;
  S.cleanup s;
  print_endline "OK"

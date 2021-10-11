open Curl

let test1 size =
  let h = init () in
  let s1 = String.make size 'd' in
  set_private h s1;
  let s2 = String.make size 'c' in
  set_httppost h [CURLFORM_CONTENT ("part", s2, DEFAULT)];
  assert (get_private h = s1);
  cleanup h

let test2 size =
  let h = init () in
  let s = String.make size 'a' in
  set_mimepost h [{encoding=CURLMIME_BINARY;headers=[];subparts=[];data=CURLMIME_DATA s}];
  let g = init () in
  cleanup h;
  cleanup g

let test3 size =
  let h = init () in
  let s = String.make size 'a' in
  set_mimepost
    h
    [
      {
        encoding=CURLMIME_BINARY;
        headers=[];
        subparts=[];
        data=CURLMIME_DATA_WITH_NAME {data=String s;name=Some "foo";filename=Some "bar"};
      }
    ];
  let g = init () in
  cleanup h;
  cleanup g

let rss () =
  let path = Printf.sprintf "/proc/%d/statm" (Unix.getpid ()) in
  try
    let ch = Scanf.Scanning.open_in path in
    let n = Scanf.bscanf ch "%_d %d" (fun x -> 4*1024*x) in Scanf.Scanning.close_in ch; n
  with exn -> Printf.eprintf "Error opening %s (%s), ignoring\n%!" path (Printexc.to_string exn); 0

let check test count leak_size =
  try
    let rss1 = rss () in
    for _i = 0 to pred count do
      test leak_size;
      Gc.compact ();
    done;
    let rss2 = rss () in
    Printf.printf "RSS %d -> %d %s\n%!" rss1 rss2 (if rss2 - rss1 < count * leak_size / 10 then "OK" else "LEAKING")
  with
    Curl.NotImplemented s -> Printf.printf "skipping test : libcurl doesn't provide %s\n%!" s

let () =
  let mb = 1024 * 1024 in
  check test1 200 mb;
  check test2 100 mb;
  check test3 100 mb;
  ()

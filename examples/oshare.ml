(*
 * oshare.ml
 *
 * Copyright (c) 2024, OCurl contributors
 *
 * Example demonstrating the curl share interface
 * This example shows how to share cookies, DNS cache, SSL sessions,
 * and connections between multiple curl handles.
 *)

let pr fmt = Printf.ksprintf print_endline fmt

(* Simple writer function to collect response data *)
let writer_to_buffer buf data =
  Buffer.add_string buf data;
  String.length data

(* Header function to show received headers *)
let header_function headers data =
  headers := data :: !headers;
  String.length data

(* Create and configure a curl handle *)
let create_handle share_handle url =
  let h = Curl.init () in
  let response_buf = Buffer.create 1024 in
  let headers = ref [] in
  
  (* Basic options *)
  Curl.set_url h url;
  Curl.set_writefunction h (writer_to_buffer response_buf);
  Curl.set_headerfunction h (header_function headers);
  Curl.set_verbose h false;
  Curl.set_followlocation h true;
  Curl.set_timeout h 30;
  
  (* Use the shared handle *)
  Curl.setopt h (Curl.CURLOPT_SHARE share_handle);
  
  (h, response_buf, headers)

(* Perform a request and show results *)
let perform_request name handle response_buf headers =
  pr "=== %s ===" name;
  let start_time = Unix.gettimeofday () in
  
  try
    Curl.perform handle;
    let end_time = Unix.gettimeofday () in
    let duration = end_time -. start_time in
    
    (* Show timing information *)
    pr "Total time: %.3f seconds" (Curl.get_totaltime handle);
    pr "Wall clock time: %.3f seconds" duration;
    pr "DNS lookup time: %.3f seconds" (Curl.get_namelookuptime handle);
    pr "Connect time: %.3f seconds" (Curl.get_connecttime handle);
    pr "Response code: %d" (Curl.get_responsecode handle);
    pr "Effective URL: %s" (Curl.get_effectiveurl handle);
    
    (* Show some headers *)
    let header_list = List.rev !headers in
    let cookie_headers = List.filter (fun h -> 
      String.length h > 10 && String.sub h 0 10 = "Set-Cookie") header_list in
    if cookie_headers <> [] then (
      pr "Set-Cookie headers:";
      List.iter (fun h -> pr "  %s" (String.trim h)) cookie_headers
    );
    
    (* Show cookies in the handle *)
    let cookies = Curl.get_cookielist handle in
    if cookies <> [] then (
      pr "Cookies in handle:";
      List.iter (fun cookie -> pr "  %s" cookie) cookies
    ) else (
      pr "No cookies in handle"
    );
    
    (* Show response size *)
    let response_size = Buffer.length response_buf in
    pr "Response size: %d bytes" response_size;
    
    (* Show first 100 characters of response if it's text *)
    let response_content = Buffer.contents response_buf in
    if response_size > 0 then (
      let preview_len = min 100 response_size in
      let preview = String.sub response_content 0 preview_len in
      pr "Response preview: %s%s" preview (if response_size > 100 then "..." else "")
    );
    
    pr ""
  with
  | Curl.CurlException (code, errno, msg) ->
      pr "Curl error: %s (code=%d, errno=%d)" msg (Curl.int_of_curlCode code) errno
  | e ->
      pr "Error: %s" (Printexc.to_string e)

(* Main demonstration function *)
let demo_share () =
  pr "=== OCurl Share Interface Demo ===\n";
  
  (* Create a share handle *)
  let share_handle = Curl.Share.init () in
  
  try
    (* Configure what to share *)
    pr "Configuring share handle...";
    Curl.Share.setopt share_handle (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
    Curl.Share.setopt share_handle (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_SSL_SESSION);
    Curl.Share.setopt share_handle (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_CONNECT);
    Curl.Share.setopt share_handle (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_COOKIE);
    pr "Share handle configured to share: DNS cache, SSL sessions, connections, and cookies\n";
    
    (* Test URLs - using the same domain to demonstrate sharing benefits *)
    let urls = [
      "https://httpbin.org/cookies/set/session/abc123";
      "https://httpbin.org/cookies";
      "https://httpbin.org/delay/1";
      "https://httpbin.org/headers";
    ] in
    
    pr "Testing with %d URLs from the same domain to demonstrate sharing benefits:\n" (List.length urls);
    
    (* Create handles for each URL *)
    let handles_and_data = List.mapi (fun i url ->
      let name = Printf.sprintf "Request %d" (i + 1) in
      let handle, buf, headers = create_handle share_handle url in
      (name, handle, buf, headers)
    ) urls in
    
    (* Perform requests sequentially to show shared data *)
    List.iter (fun (name, handle, buf, headers) ->
      perform_request name handle buf headers;
      (* Small delay to make the demo more readable *)
      Unix.sleep 1
    ) handles_and_data;
    
    pr "=== Sharing Benefits Demonstrated ===";
    pr "- DNS cache: Same domain lookups should be faster after the first request";
    pr "- SSL sessions: SSL handshake time should be reduced for subsequent HTTPS requests";
    pr "- Connections: HTTP connections can be reused when possible";
    pr "- Cookies: Cookies set by one handle are available to others";
    pr "";
    
    (* Clean up curl handles *)
    List.iter (fun (_, handle, _, _) -> Curl.cleanup handle) handles_and_data;
    
    (* Clean up share handle *)
    Curl.Share.cleanup share_handle;
    pr "Share handle cleaned up successfully"
    
  with
  | Curl.CurlException (code, errno, msg) ->
      Curl.Share.cleanup share_handle;
      pr "Curl error: %s (code=%d, errno=%d)" msg (Curl.int_of_curlCode code) errno
  | e ->
      Curl.Share.cleanup share_handle;
      pr "Error: %s" (Printexc.to_string e)

(* Cookie sharing validation demo *)
let demo_manual_cookie_share () =
  pr "\n=== Cookie Sharing Validation Demo ===\n";
  
  (* Part 1: Demonstrate manual cookie injection WITHOUT sharing *)
  pr "=== Part 1: WITHOUT Cookie Sharing ===";
  let h1_no_share = Curl.init () in
  let h2_no_share = Curl.init () in
  let ()=
  try
    (* Manually inject a cookie into handle 1 *)
    Curl.set_cookielist h1_no_share "example.com\tFALSE\t/\tFALSE\t0\ttest_cookie\tvalue123";
    
    let cookies1 = Curl.get_cookielist h1_no_share in
    let cookies2 = Curl.get_cookielist h2_no_share in
    
    pr "Handle 1 - After manual cookie injection:";
    pr "  Cookies in handle 1: %d cookies" (List.length cookies1);
    List.iter (fun cookie -> pr "    %s" cookie) cookies1;
    
    pr "Handle 2 - Independent handle:";
    pr "  Cookies in handle 2: %d cookies" (List.length cookies2);
    List.iter (fun cookie -> pr "    %s" cookie) cookies2;
    
    if List.length cookies1 > 0 && List.length cookies2 = 0 then
      pr "✓ WITHOUT sharing: Handle 1 has cookies, Handle 2 has none (as expected)"
    else
      pr "? WITHOUT sharing: Unexpected cookie distribution";
    
    Curl.cleanup h1_no_share;
    Curl.cleanup h2_no_share;
    
  with
  | e ->
      Curl.cleanup h1_no_share;
      Curl.cleanup h2_no_share;
      pr "Error in non-sharing demo: %s" (Printexc.to_string e);
  in
  pr "\n=== Part 2: WITH Cookie Sharing ===";
  let share_handle = Curl.Share.init () in
  let h1_share = ref None in
  let h2_share = ref None in
  let ()=
  try
    (* Configure share handle *)
    Curl.Share.setopt share_handle (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_COOKIE);
    pr "Share handle configured to share cookies only\n";
    
    (* Create handles with sharing *)
    let h1 = Curl.init () in
    let h2 = Curl.init () in
    h1_share := Some h1;
    h2_share := Some h2;
    
    (* Associate handles with share *)
    Curl.setopt h1 (Curl.CURLOPT_SHARE share_handle);
    Curl.setopt h2 (Curl.CURLOPT_SHARE share_handle);
    
    pr "Handle 1 - Injecting cookie with sharing enabled:";
    Curl.set_cookielist h1 "shared.com\tFALSE\t/\tFALSE\t0\tshared_cookie\tshared_value456";
    
    let shared_cookies1 = Curl.get_cookielist h1 in
    let shared_cookies2 = Curl.get_cookielist h2 in
    
    pr "  Cookies in handle 1: %d cookies" (List.length shared_cookies1);
    List.iter (fun cookie -> pr "    %s" cookie) shared_cookies1;
    pr "  Cookies in handle 2: %d cookies" (List.length shared_cookies2);
    List.iter (fun cookie -> pr "    %s" cookie) shared_cookies2;
    
    if List.length shared_cookies1 > 0 && List.length shared_cookies2 > 0 then
      pr "✓ WITH sharing: Both handles have cookies (sharing successful!)"
    else if List.length shared_cookies1 > 0 && List.length shared_cookies2 = 0 then
      pr "? WITH sharing: Cookie not visible in second handle (sharing may not be working as expected)"
    else
      pr "? WITH sharing: Unexpected cookie distribution";
    
    (* Additional validation: inject into handle 2 and check handle 1 *)
    pr "\n=== Part 3: Reverse Cookie Injection Test ===";
    Curl.set_cookielist h2 "reverse.com\tFALSE\t/\tFALSE\t0\treverse_test\treverse_value";
    
    let reverse_cookies1 = Curl.get_cookielist h1 in
    let reverse_cookies2 = Curl.get_cookielist h2 in
    
    pr "After injecting cookie into handle 2:";
    pr "  Handle 1 cookies: %d" (List.length reverse_cookies1);
    List.iter (fun cookie -> pr "    %s" cookie) reverse_cookies1;
    pr "  Handle 2 cookies: %d" (List.length reverse_cookies2);
    List.iter (fun cookie -> pr "    %s" cookie) reverse_cookies2;
    
    if List.length reverse_cookies1 = List.length reverse_cookies2 then
      pr "✓ Reverse injection: Both handles have same number of cookies (sharing works!)"
    else
      pr "? Reverse injection: Cookie counts differ (sharing may be one-way only)";
    
    (* Proper cleanup order: dissociate handles from share first *)
    pr "\nCleaning up handles...";
    (match !h1_share with Some h -> Curl.cleanup h | None -> ());
    (match !h2_share with Some h -> Curl.cleanup h | None -> ());
    h1_share := None;
    h2_share := None;
    
    pr "Cleaning up share handle...";
    Curl.Share.cleanup share_handle;
    pr "✓ Cookie sharing validation demo completed successfully"
    
  with
  | e ->
      pr "Error in sharing demo: %s" (Printexc.to_string e);
      (* Clean up in reverse order *)
      (match !h1_share with Some h -> (try Curl.cleanup h with _ -> ()) | None -> ());
      (match !h2_share with Some h -> (try Curl.cleanup h with _ -> ()) | None -> ());
      Curl.Share.cleanup share_handle
  in
  ()
(* Main function *)
let () =
  Curl.global_init Curl.CURLINIT_GLOBALALL;
  
  try
    (* Run the main demo *)
    demo_share ();
    
    (* Run the cookie sharing validation demo *)
    demo_manual_cookie_share ();
    
    pr "\nAll demos completed successfully!";
    Curl.global_cleanup ()
  with
  | e ->
      pr "Fatal error: %s" (Printexc.to_string e);
      Curl.global_cleanup () 
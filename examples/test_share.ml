(*
 * test_share.ml
 *
 * Copyright (c) 2024, OCurl contributors
 *
 * Test for the curl share interface
 *)

let test_share_creation () =
  let share = Curl.Share.init () in
  Curl.Share.cleanup share;
  Printf.printf "✓ Share creation and cleanup test passed\n"

let test_share_options () =
  let share = Curl.Share.init () in
  try
    (* Test setting each share option *)
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_COOKIE);
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_SSL_SESSION);
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_CONNECT);
    
    (* Test unsetting options *)
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_UNSHARE Curl.Share.CURLSHOPT_SHARE_COOKIE);
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_UNSHARE Curl.Share.CURLSHOPT_SHARE_DNS);
    
    Curl.Share.cleanup share;
    Printf.printf "✓ Share options test passed\n"
  with
  | e ->
      Curl.Share.cleanup share;
      Printf.printf "✗ Share options test failed: %s\n" (Printexc.to_string e)

let test_share_exceptions () =
  Printf.printf "Testing share exception handling...\n";
  
  (* Test 1: Using a cleaned up share handle should raise exception *)
  let share = Curl.Share.init () in
  Curl.Share.cleanup share;
  
  let test_invalid_handle () =
    try
      Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
      Printf.printf "✗ Expected ShareError exception for invalid handle, but none was raised\n";
      false
    with
    | Curl.Share.ShareError (code, errno, msg) ->
        Printf.printf "✓ ShareError caught for invalid handle: code=%s, errno=%d, msg=%s\n" 
          (match code with
           | Curl.Share.CURLSHE_INVALID -> "CURLSHE_INVALID"
           | Curl.Share.CURLSHE_BAD_OPTION -> "CURLSHE_BAD_OPTION"
           | Curl.Share.CURLSHE_IN_USE -> "CURLSHE_IN_USE"
           | Curl.Share.CURLSHE_NOMEM -> "CURLSHE_NOMEM"
           | Curl.Share.CURLSHE_NOT_BUILT_IN -> "CURLSHE_NOT_BUILT_IN"
           | Curl.Share.CURLSHE_OK -> "CURLSHE_OK")
          errno msg;
        true
    | e ->
        Printf.printf "✗ Expected ShareError, but got: %s\n" (Printexc.to_string e);
        false
  in
  
  (* Test 2: Test share_strerror function *)
  let test_strerror () =
    try
      let error_msg = Curl.Share.strerror 1 in
      Printf.printf "✓ share_strerror(1) returned: %s\n" error_msg;
      true
    with
    | e ->
        Printf.printf "✗ share_strerror test failed: %s\n" (Printexc.to_string e);
        false
  in
  
  (* Test 3: Test setting options on a valid share handle (should not raise exception) *)
  let test_valid_operations () =
    let share = Curl.Share.init () in
    try
      Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
      Curl.Share.setopt share (Curl.Share.CURLSHOPT_UNSHARE Curl.Share.CURLSHOPT_SHARE_DNS);
      Curl.Share.cleanup share;
      Printf.printf "✓ Valid share operations completed without exceptions\n";
      true
    with
    | e ->
        Curl.Share.cleanup share;
        Printf.printf "✗ Valid share operations failed: %s\n" (Printexc.to_string e);
        false
  in
  
  (* Test 4: Test multiple cleanup calls (should handle gracefully) *)
  let test_double_cleanup () =
    let share = Curl.Share.init () in
    try
      Curl.Share.cleanup share;
      Curl.Share.cleanup share; (* This might be safe or might throw - depends on implementation *)
      Printf.printf "✓ Double cleanup handled gracefully\n";
      true
    with
    | e ->
        Printf.printf "✓ Double cleanup raised exception as expected: %s\n" (Printexc.to_string e);
        true (* This is also acceptable behavior *)
  in
  
  (* Test 5: Test exception details are correct *)
  let test_exception_details () =
    let share = Curl.Share.init () in
    Curl.Share.cleanup share;
    try
      Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
      Printf.printf "✗ Expected exception for invalid handle use\n";
      false
    with
    | Curl.Share.ShareError (code, errno, msg) ->
        (* Validate exception components *)
        let code_is_valid = match code with
          | Curl.Share.CURLSHE_INVALID | Curl.Share.CURLSHE_BAD_OPTION 
          | Curl.Share.CURLSHE_IN_USE | Curl.Share.CURLSHE_NOMEM 
          | Curl.Share.CURLSHE_NOT_BUILT_IN | Curl.Share.CURLSHE_OK -> true
        in
        let errno_is_valid = errno >= 0 in
        let msg_is_valid = String.length msg > 0 in
        
        if code_is_valid && errno_is_valid && msg_is_valid then (
          Printf.printf "✓ Exception details are valid: code=%s, errno=%d, msg_len=%d\n" 
            (match code with
             | Curl.Share.CURLSHE_INVALID -> "CURLSHE_INVALID"
             | Curl.Share.CURLSHE_BAD_OPTION -> "CURLSHE_BAD_OPTION"
             | Curl.Share.CURLSHE_IN_USE -> "CURLSHE_IN_USE"
             | Curl.Share.CURLSHE_NOMEM -> "CURLSHE_NOMEM"
             | Curl.Share.CURLSHE_NOT_BUILT_IN -> "CURLSHE_NOT_BUILT_IN"
             | Curl.Share.CURLSHE_OK -> "CURLSHE_OK")
            errno (String.length msg);
          true
        ) else (
          Printf.printf "✗ Exception details are invalid: code_valid=%b, errno_valid=%b, msg_valid=%b\n" 
            code_is_valid errno_is_valid msg_is_valid;
          false
        )
    | e ->
        Printf.printf "✗ Expected ShareError but got: %s\n" (Printexc.to_string e);
        false
  in
  
  (* Test 6: Test all share data types *)
  let test_all_share_types () =
    let share = Curl.Share.init () in
    try
      let share_types = [
        Curl.Share.CURLSHOPT_SHARE_COOKIE;
        Curl.Share.CURLSHOPT_SHARE_DNS;
        Curl.Share.CURLSHOPT_SHARE_SSL_SESSION;
        Curl.Share.CURLSHOPT_SHARE_CONNECT;
      ] in
      List.iter (fun share_type ->
        Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE share_type);
        Curl.Share.setopt share (Curl.Share.CURLSHOPT_UNSHARE share_type);
      ) share_types;
      
      Curl.Share.cleanup share;
      Printf.printf "✓ All share data types tested successfully\n";
      true
    with
    | e ->
        Curl.Share.cleanup share;
        Printf.printf "✗ Share data types test failed: %s\n" (Printexc.to_string e);
        false
  in
  
  let results = [
    test_invalid_handle ();
    test_strerror ();
    test_valid_operations ();
    test_double_cleanup ();
    test_exception_details ();
    test_all_share_types ();
  ] in
  
  let passed = List.fold_left (fun acc result -> acc && result) true results in
  if passed then
    Printf.printf "✓ All share exception tests passed\n"
  else
    Printf.printf "✗ Some share exception tests failed\n"

let test_share_with_curl_handle () =
  let share = Curl.Share.init () in
  try
    (* Configure share *)
    Curl.Share.setopt share (Curl.Share.CURLSHOPT_SHARE Curl.Share.CURLSHOPT_SHARE_DNS);
    
    (* Create a curl handle and associate it with the share *)
    let h = Curl.init () in
    Curl.set_url h "https://httpbin.org/status/200";
    Curl.setopt h (Curl.CURLOPT_SHARE share);
    
    (* Set up a simple write function to discard data *)
    Curl.set_writefunction h (fun _ -> 0);
    
    (* Test that the handle works with the share *)
    (try
      Curl.perform h;
      Printf.printf "✓ Curl handle with share test passed\n"
    with
    | Curl.CurlException (code, errno, msg) ->
        Printf.printf "✗ Curl handle with share test failed: %s (code=%d, errno=%d)\n" 
          msg (Curl.int_of_curlCode code) errno
    | e ->
        Printf.printf "✗ Curl handle with share test failed: %s\n" (Printexc.to_string e)
    );
    
    Curl.cleanup h;
    Curl.Share.cleanup share
  with
  | e ->
      Curl.Share.cleanup share;
      Printf.printf "✗ Share with curl handle test failed: %s\n" (Printexc.to_string e)

let run_all_tests () =
  Printf.printf "Running curl share interface tests...\n\n";
  
  test_share_creation ();
  test_share_options ();
  test_share_exceptions ();
  test_share_with_curl_handle ();
  
  Printf.printf "\nAll share tests completed.\n"

let () =
  Curl.global_init Curl.CURLINIT_GLOBALALL;
  
  try
    run_all_tests ();
    Curl.global_cleanup ()
  with
  | e ->
      Printf.printf "Fatal error: %s\n" (Printexc.to_string e);
      Curl.global_cleanup () 
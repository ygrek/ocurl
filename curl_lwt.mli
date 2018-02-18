(** Asynchronous API with Lwt *)

(**
  perform [Curl.t] asynchronously
  @return transfer result code
*)
val perform : Curl.t -> Curl.curlCode Lwt.t

(**
   set option on global multi_handle
*)
val setopt : Curl.Multi.curlMultiOption -> unit

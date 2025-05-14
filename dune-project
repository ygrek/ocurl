(lang dune 3.0)
(name curl)

(formatting disabled)
(generate_opam_files true)

(maintainers
 "ygrek@autistici.org"
 "Antonin Décimo <antonin@tarides.com>")
(authors
 "Lars Nilsson"
 "ygrek")
(source
 (github ygrek/ocurl))
(license MIT)
(homepage "https://ygrek.org/p/ocurl")
(documentation "https://ygrek.org/p/ocurl/api/index.html")

(package
 (name curl)
 (synopsis "Bindings to libcurl")
 (description "libcurl is a client-side URL transfer library, supporting HTTP and a multitude of other network protocols (FTP/SMTP/RTSP/etc). This library wrap easy synchronous API (Curl) and synchronous parallel and generic asynchronous API (Curl.Multi). For the Lwt-enabled asynchronous interface (Curl_lwt) see curl-lwt package.")
 (depends
  dune
  (ocaml (>= "4.11"))
  (dune-configurator (>= "3.18.1"))
  base-bigarray
  base-unix  
  conf-libcurl)
 (conflicts
  (ocurl (<> "transition")))
 (tags (org:ygrek clib:curl)))

(package
 (name curl-lwt)
 (synopsis "Bindings to libcurl (lwt variant)")
 (description "libcurl is a client-side URL transfer library, supporting HTTP and a multitude of other network protocols (FTP/SMTP/RTSP/etc). This library  provides an Lwt-enabled asynchronous interface (Curl_lwt).")
 (depends
  dune
  (ocaml (>= "4.11"))
  base-unix
  (curl (= :version))
  lwt
  (lwt_ppx :with-dev-setup))
 (tags (org:ygrek clib:curl)))

(package
 (name ocurl)
 (synopsis "This is a transition package, ocurl is now named curl. Use the curl package instead")
 (description "This is a transition package, ocurl is now named curl. Use the curl package instead.")
 (allow_empty)
 (version transition))

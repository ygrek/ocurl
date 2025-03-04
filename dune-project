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
 (description "libcurl is a client-side URL transfer library, supporting HTTP and a multitude of other network protocols (FTP/SMTP/RTSP/etc). This library wrap easy synchronous API (Curl) and synchronous parallel and generic asynchronous API (Curl.Multi). For the Lwt-enabled asynchronous interface (Curl_lwt) see curl_lwt package.")
 (depends
  (ocaml (>= "4.03.0"))
  base-unix
  conf-libcurl)
 (tags (org:ygrek clib:curl)))

(package
 (name curl_lwt)
 (synopsis "Bindings to libcurl (lwt variant)")
 (description "libcurl is a client-side URL transfer library, supporting HTTP and a multitude of other network protocols (FTP/SMTP/RTSP/etc). This library  provides an Lwt-enabled asynchronous interface (Curl_lwt).")
 (depends
  (ocaml (>= "4.03.0"))
  base-unix
  (curl (= :version))
  lwt)
 (tags (org:ygrek clib:curl)))

(package
 (name ocurl)
 (synopsis "Transitional dummy package: ocurl -> curl")
 (description "Use `opam install curl`")
 (depends
  (curl (= :version)))
 (allow_empty))

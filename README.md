# ocurl - OCaml libcurl bindings

[![Build Status](https://github.com/ygrek/ocurl/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/ygrek/ocurl/actions/workflows/main.yml?branch=master)
[![OCaml-CI Build Status](https://img.shields.io/endpoint?url=https%3A%2F%2Focaml.ci.dev%2Fbadge%2Fygrek%2Focurl%2Fmaster&logo=ocaml)](https://ocaml.ci.dev/github/ygrek/ocurl)

Homepage: https://ygrek.org/p/ocurl

OCaml bindings to libcurl - client-side URL transfer library, supporting
HTTP and a multitude of other network protocols. This is a continuation
of the ocurl project by Lars Nilsson, previously hosted at
<https://sourceforge.net/projects/ocurl/>.

NB: the historical project name is `ocurl`, but the opam package and the
OCaml library name is `curl` (there exists a transitional dummy `ocurl`
opam package for compatibility).

Minimum supported libcurl version: 7.28.0 (Oct 10 2012).

## Build

``` shell
dune build
# or
make
```

## Adding new libcurl symbol

1. Add the new symbol in [`config/discover.ml`](./config/discover.ml);
2. Edit [`curl-helper.c`](./curl-helper.c) and [`curl.ml`](./curl.ml),
   [`curl.mli`](./curl.mli) accordingly.

----
 ygrek at autistici dot org

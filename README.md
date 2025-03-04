ocurl - OCaml libcurl bindings
==============================

[![Build Status](https://github.com/ygrek/ocurl/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/ygrek/ocurl/actions/workflows/main.yml?branch=master)
[![OCaml-CI Build Status](https://img.shields.io/endpoint?url=https%3A%2F%2Focaml.ci.dev%2Fbadge%2Fygrek%2Focurl%2Fmaster&logo=ocaml)](https://ocaml.ci.dev/github/ygrek/ocurl)

Homepage: https://ygrek.org/p/ocurl

OCaml bindings to libcurl - client-side URL transfer library,
supporting HTTP and a multitude of other network protocols.
This is a continuation of ocurl project by Lars Nilsson,
previously hosted at http://ocurl.sourceforge.net/

NB historically project name is `ocurl`, but opam package and ocaml library name is `curl` (there exists transitional dummy `ocurl` opam package for compatibility).

Minimum supported libcurl version : 7.28.0

Build
=====

  ./configure && make

Adding new libcurl symbol
=========================

* add symbol in configure.ac
* autoreconf
* make clean
* ./configure
* edit curl-helper.c and curl.ml*

Making release
==============

* Check `make gen` with latest libcurl
* Update CHANGES.txt
* Update version in configure.ac
* autoreconf
* commit
* ./configure && make && make release

TODO:
use dune-release (Update CHANGES.md: replace the first heading by release number)

----
 ygrek at autistici dot org

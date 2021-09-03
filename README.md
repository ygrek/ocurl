ocurl - OCaml libcurl bindings
==============================

[![Build Status](https://travis-ci.org/ygrek/ocurl.svg?branch=master)](https://travis-ci.org/ygrek/ocurl) [![Build status](https://ci.appveyor.com/api/projects/status/b20uqxaeyarwy2s4/branch/master?svg=true)](https://ci.appveyor.com/project/ygrek/ocurl/branch/master)

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

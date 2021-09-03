ocurl - OCaml libcurl bindings
==============================

[![Build Status](https://travis-ci.org/ygrek/ocurl.svg?branch=master)](https://travis-ci.org/ygrek/ocurl) [![Build status](https://ci.appveyor.com/api/projects/status/b20uqxaeyarwy2s4/branch/master?svg=true)](https://ci.appveyor.com/project/ygrek/ocurl/branch/master)

Homepage: https://ygrek.org.ua/p/ocurl

OCaml bindings to libcurl - client-side URL transfer library,
supporting HTTP and a multitude of other network protocols.
This is a continuation of ocurl project by Lars Nilsson,
previously hosted at http://ocurl.sourceforge.net/

Minimum supported libcurl version : 7.28.0

Adding new libcurl symbol
=========================

* add symbol in configure.ac
* autoreconf
* make clean
* ./configure
* edit curl-helper.c and curl.ml*

Making release to OPAM
======================

* Check `make gen` with latest libcurl
* Update CHANGES.md: replace the first heading by release number
* autoreconf
* commit
* ./configure && make && make release

----
 ygrek at autistici dot org

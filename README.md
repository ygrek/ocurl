
ocurl - OCaml libcurl bindings
==============================

[![Build Status](https://travis-ci.org/ygrek/ocurl.svg?branch=master)](https://travis-ci.org/ygrek/ocurl) [![Build status](https://ci.appveyor.com/api/projects/status/b20uqxaeyarwy2s4/branch/master?svg=true)](https://ci.appveyor.com/project/ygrek/ocurl/branch/master)

Homepage: http://ygrek.org.ua/p/ocurl/

OCaml bindings to libcurl - client-side URL transfer library,
supporting HTTP and a multitude of other network protocols.
This is a continuation of ocurl project by Lars Nilsson,
previously hosted at http://ocurl.sourceforge.net/

Minimum supported libcurl version : 7.28.0

Building on Windows with ocaml/msvc
===================================

Requirements
------------

  * libcurl devel for msvc <http://curl.haxx.se/latest.cgi?curl=win32-ssl-devel-msvc>
  * working ocaml/msvc setup (ocaml and msvc tools in PATH)
  * ocamlfind
  * GNU make

Build
-----

Edit `Makefile.msvc` and make CURLDIR variable point
to the root directory of libcurl installation.
Copy `config.h.windows` to `config.h`.

Compile with `make -f Makefile.msvc`.
Install with `make -f Makefile.msvc install`.
Compile examples with `make -f Makefile.msvc examples`.

Making release
==============

* Update CHANGES.txt
* Update version in configure.ac
* autoreconf
* commit
* ./configure && make release

----
 ygrek at autistici dot org

## Working version

- Switch to Dune

## 0.9.1  - 13 Feb 2020

+ HTTP_VERSION_3
* fix build with OCaml 4.10

## 0.9.0  - 13 Nov 2019

* remove duphandle
* fix Curl.pause segfault with libcurl >= 7.60.0

## 0.8.2  -  5 Aug 2018

* deprecate duphandle
+ curlCode_of_int
+ CURLOPT_MIMEPOST
+ CURLOPT_SSH_KNOWNHOSTS CURLOPT_SSH_KEYFUNCTION
+ HTTP_VERSION_2_PRIOR_KNOWLEDGE

## 0.8.1  - 11 Mar 2018

+ CURLOPT_POSTREDIR
* fix Multi.wait on windows (Nicolas Ojeda Bar)
* require libcurl >= 7.28.0

## 0.8.0  - 28 Nov 2017

* fix build on Windows/msvc (Nicolas Ojeda Bar)
* fix debugfunction invocation
* require libcurl >= 7.17.0
* less mallocs per handle
+ CURLOPT_USERNAME CURLOPT_PASSWORD CURLOPT_LOGIN_OPTIONS CURLOPT_CONNECT_TO
+ build cmxs
* fix build tests with OCaml 4.06.0

## 0.7.10 - 12 Jun 2017

+ CURL_SSLVERSION_TLSv1_3
* O(1) vs O(N) speedup for workloads with lots of short-lived concurrent connections

## 0.7.9  -  3 Oct 2016

* fix build with older libcurl

## 0.7.8  -  6 Sep 2016

+ CURLOPT_PIPEWAIT
+ CURLOPT_CERTINFO CURLINFO_CERTINFO
+ CURL_HTTP_VERSION_2 CURL_HTTP_VERSION_2TLS
* CURLOPT_SSLVERSION is now a variant type (backward-incompatible change)

## 0.7.7  -  16 May 2016

+ Multi.setopt

## 0.7.6  -  25 Oct 2015

* fix invalid memory access in CURLOPT_HTTPPOST handling (mfp)

## 0.7.5  -  24 Jul 2015

* fix build with older libcurl

## 0.7.4  -  23 Jun 2015

* fix build

## 0.7.3  -  22 Jun 2015

+ CURLOPT_MAIL_FROM CURLOPT_MAIL_RCPT
- CURLOPT_STDERR
* fix CURLOPT_SSH_HOST_PUBLIC_KEY_MD5 and CURLOPT_INFILESIZE_LARGE
* fix memory leaks in CURLOPT_HTTPPOST handling
* use specific NotImplemented exception instead of generic Failure

## 0.7.2  -  23 Sep 2014

* fix Curl.duphandle wrt CURLOPT_DNS_SERVERS
* lwt: fix memory leak
* Multi: keep Curl.t alive

## 0.7.1  -  12 May 2014

* Multi: win32 support (arirux)
+ Multi.remove
* lwt: handle Lwt.cancel
* lwt: fix set_errorbuffer

## 0.7.0  -  8 Mar 2014

* Curl_lwt: basic Lwt interface
* make Curl.t a custom value (with compare and hash)
* set_readfunction: assert correct length, do not silently truncate data
* generate ocamldoc html with `make doc`
* fix build: set CFLAGS for feature tests

## 0.6.1  -  11 Feb 2014

* fix type of set_seekfunction
* fix handling of exceptions from callbacks (break the transfer)
+ CURLINFO_CONDITION_UNMET TIMECOND_NONE TIMECOND_LASTMOD
* fix build on windows
* expose Curl.t underlying Curl.handle object
* implement Curl.handle#get_redirecturl

## 0.6.0  -  29 Aug 2013

* introduce bindings to asynchronous multi interface
+ Multi: set_socket_function set_timer_function action_all action_timeout action timeout
* treat SSLVERIFYHOST_EXISTENCE as SSLVERIFYHOST_HOSTNAME, previous workaround was broken
* MSVC compatibility
* configure: do not override CFLAGS
* expose set_sshprivatekeyfile

## 0.5.6  -  21 Mar 2013

* configure: more robust test for libcurl

## 0.5.5  -  23 Feb 2013

+ CURLINFO_LOCAL_IP CURLINFO_LOCAL_PORT
+ pause
+ compatibility fixes for old libcurl versions

## 0.5.4  -  29 Jan 2013

- Makefile: add release target
- add CHANGES.txt
- Makefile: add distclean target

# Older changes

## 2012

- version_info: return features as list of strings
- more version info fields
- replace SSLVERIFYHOST_EXISTENCE with SSLVERIFYHOST_HOSTNAME for new libcurl
+ CURLOPT_RESOLVE CURLOPT_DNS_SERVERS
+ CURLOPT_PROTOCOLS and CURLOPT_REDIR_PROTOCOLS
- update Copyright
- fix free_curl_slist (crash with 7.24.0)
- Makefile: add uninstall target
- update ocaml m4 macros (in particular ocamlfind will be used by default, if present)
+ CURLINFO_PRIMARY_IP

## 2011

+ CURLOPT_PROXYTYPE
+ CURLOPT_OPENSOCKETFUNCTION
+ SSLVERIFYHOST_NONE
- support CURLOPT_AUTOREFERER

## 2010

+ version_info
- fix: memory leak in curl_slist handling
- disable checkConnection (kills performance with many handles)
- support CURLINFO_REDIRECT_URL
- tabs -> spaces
- fix: double free (connection->range)
- support all encodings
- fix: CURLINFO_FILETIME has type long
- add errno
- add strerror, return curlCode from remove_finished
- actually retrieve CURLINFO_FTP_ENTRY_PATH
- fix typo HAVE_DECL_CURLINFO_HTTP_CONNECTCODE
- remove CURLE_FTP_SSL_FAILED (breaks int to curlCode conversion)
- add README for ocaml/msvc build
- fix crash bug (use Store_field) in raiseError
- fix: do not use Store_field on Abstract_tag blocks
- install with ocamlfind on windows too
- fix ocamlfind installation
- wrap CURLM* as custom value
- fix build (build dllcurl-helper.dll once)
- link with ws2_32.dll for select for curlm_wait_data
- use explicit -dllpath in examples (so that examples work without installing)
- start use ocamlmklib
- tweak code and Makefile for msvc build

## 2009

- test code for threads
+ Curl.reset
- fix crash: NULL from curl_easy_getinfo
- fix: helper_* functions should return Val_unit
- examples/omulti: accept command-line args
- build dllcurl-helper (partially merged deb patch)
+ examples/omulti
- fix crash bug (use Store_field)
+ Multi.{add,perform,wait}
+ Connection_val
+ Curl.Multi.remove_finished
- make -Wall happier
- fix error in seekFunction
- start Curl.Multi
- fix locking in callbacks
- start from ocurl 0.5.1

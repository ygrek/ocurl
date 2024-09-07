.PHONY: all
all:
	dune build @install

.PHONY: test
test:
	dune runtest

.PHONY: doc
doc:
	dune build @doc

PREFIX_ARG := $(if $(PREFIX),--prefix $(PREFIX),)
LIBDIR_ARG := $(if $(LIBDIR),--libdir $(LIBDIR),)
DESTDIR_ARG := $(if $(DESTDIR),--destdir $(DESTDIR),)
INSTALL_ARGS := $(PREFIX_ARG) $(LIBDIR_ARG) $(DESTDIR_ARG)

.PHONY: install
install:
	dune install $(INSTALL_ARGS)

.PHONY: uninstall
uninstall:
	dune uninstall $(INSTALL_ARGS)

.PHONY: reinstall
reinstall: uninstall install

.PHONY: clean
clean:
	dune clean

.PHONY: distclean
distclean: clean
	rm -f config.log config.status config.h Makefile dune-project clibs.sexp cflags.sexp # AC_CONFIG_FILES

.PHONY: gen
gen:
	dune exec -- examples/test_enum.exe

PACKAGE_NAME=ocurl
PACKAGE_VERSION=$(shell sed -n 's#(version \([0-9.]+\))#\1#p' dune-project)
NAME=$(PACKAGE_NAME)-$(PACKAGE_VERSION)

.PHONY: release
release:
	git tag -a -m $(PACKAGE_VERSION) $(PACKAGE_VERSION)
	git archive --prefix=$(NAME)/ $(PACKAGE_VERSION) | gzip > $(NAME).tar.gz
	gpg -a -b $(NAME).tar.gz -o $(NAME).tar.gz.asc


TODAY=$(shell date '+%d %b %Y')

.PHONY: distrib
dune-distrib:
	sed -i 's;## Working version;## $(PACKAGE_VERSION) - $(TODAY);' CHANGES.md
	git commit -m "$(PACKAGE_VERSION)" CHANGES.md
	dune-release tag
	dune-release --skip-build

.PHONY: publish
dune-publish: dune-distrib
	dune-release publish distrib

.PHONY: release
dune-release: dune-publish
	dune-release opam pkg
	dune-release opam submit

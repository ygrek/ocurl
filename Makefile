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

.PHONY: gen
gen:
	dune exec -- examples/test_enum.exe

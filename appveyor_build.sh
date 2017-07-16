#!/bin/bash

function run {
    NAME=$1
    shift
    echo "-=-=- $NAME -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
    "$@"
    CODE=$?
    if [ $CODE -ne 0 ]; then
        echo "-=-=- $NAME failed! -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
        exit $CODE
    else
        echo "-=-=- End of $NAME -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
    fi
}

echo ** OCAMLROOT=$OCAMLROOT

cd $APPVEYOR_BUILD_FOLDER

tar -xzf flexdll.tar.gz
cd flexdll-0.35
make MSVC_DETECT=0 CHAINS=msvc64 support
cp flexdll*_msvc64.obj "$FLEXDLL_DIR"
cd ..

# Do not perform end-of-line conversion
git config --global core.autocrlf false
git clone https://github.com/ocaml/ocaml.git --branch $OCAMLBRANCH --depth 1 ocaml

cd ocaml

MAKEOCAML=make
CONFIG_DIR=byterun/caml

case $OCAMLBRANCH in
    4.01|4.02|4.03|4.04|4.05)
        MAKEOCAML="make -f Makefile.nt"
        CONFIG_DIR=config
        ;;
esac

if [ ! -f $OCAMLROOT/STAMP ] || [ "$(git rev-parse HEAD)" != "$(cat $OCAMLROOT/STAMP)" ]; then
    cp config/m-nt.h $CONFIG_DIR/m.h
    cp config/s-nt.h $CONFIG_DIR/s.h
    #cp config/Makefile.msvc config/Makefile
    cp config/Makefile.msvc64 config/Makefile

    cp config/Makefile config/Makefile.bak
    sed -e "s|PREFIX=.*|PREFIX=$OCAMLROOT|" \
        -e "s|WITH_DEBUGGER=.*|WITH_DEBUGGER=|" \
        -e "s|WITH_OCAMLDOC=.*|WITH_OCAMLDOC=|" \
        config/Makefile.bak > config/Makefile
    #run "Content of config/Makefile" cat config/Makefile

    run "make world" $MAKEOCAML world
    run "make opt" $MAKEOCAML opt
    run "make install" $MAKEOCAML install

    git rev-parse HEAD > $OCAMLROOT/STAMP
fi

export CAML_LD_LIBRARY_PATH=$OCAMLROOT/lib/stublibs

cd $CURLDIR/winbuild

run "make libcurl x64" nmake /f Makefile.vc MODE=dll MACHINE=x64

cd $APPVEYOR_BUILD_FOLDER

cp Makefile.msvc Makefile.msvc.bak

sed -e "s|CURLDIR=.*|CURLDIR=$CURLDIR/builds/libcurl-vc-x64-release-dll-ipv6-sspi-winssl|" \
    Makefile.msvc.bak > Makefile.msvc

cp config.h.windows config.h

run "Contents of ocurl/Makefile.msvc" cat Makefile.msvc

run "make all" make -f Makefile.msvc targets

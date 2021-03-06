#!/bin/sh -e

XCA_DIR="`pwd`"/xca

OPENSC_DIR="opensc-0.11.13"
OPENSC_GZ="${OPENSC_DIR}".tar.gz
OPENSC_DL="http://www.opensc-project.org/files/opensc/${OPENSC_GZ}"

LIBTOOL_DIR="libtool-2.2.6b"
LIBTOOL_GZ="${LIBTOOL_DIR}".tar.gz
LIBTOOL_DL="http://ftp.gnu.org/gnu/libtool/${LIBTOOL_GZ}"

OPENSSL_DIR="openssl-1.0.0a"
OPENSSL_GZ="${OPENSSL_DIR}".tar.gz
OPENSSL_DL="http://openssl.org/source/${OPENSSL_GZ}"
OPENSSL_PATCH="$XCA_DIR/misc/openssl-1.0.0-mingw32-cross.patch"


unpack() {
  eval "dir=\${$1_DIR} gz=\${$1_GZ} dl=\${$1_DL} PATCH=\${$1_PATCH}"
  test -f "$gz" || curl "$dl" -o "$gz"
  echo "Building '$dir'"
  rm -rf "$dir"
  tar -zxf "$gz"
  cd "$dir"
  if test -f "$PATCH"; then
    patch -p1 < "$PATCH"
    test ! -x bootstrap || ./bootstrap
  fi
}

do_openssl()
{(
unpack OPENSSL
sh ms/mingw32-cross.sh
)}

do_libtool()
{(
unpack LIBTOOL
./configure --host i586-mingw32msvc --prefix ${INSTALL_DIR}
make && make install
)}

do_OpenSC()
{(
unpack OPENSC
CFLAGS=-I${INSTALL_DIR}/include LDFLAGS=-L${INSTALL_DIR}/lib LIBP11_CFLAGS=${CFLAGS} OPENSSL_CFLAGS=${CFLAGS} LIBP11_LIBS="-lp11" OPENSSL_LIBS="-leay32" ./configure --host i586-mingw32msvc --prefix ${INSTALL_DIR} --enable-openssl \
                --disable-zlib \
                --disable-readline \
                --disable-iconv \
                --enable-openssl \
                --disable-openct \
                --enable-pcsc \
                --disable-nsplugin \
                --disable-man \
                --disable-doc \
                --with-cygwin-native \
                --without-xsl-stylesheetsdir \
                --without-plugindir \
                --without-pinentry

make && make install
)}

do_XCA()
{(
  cd $XCA_DIR
  ./configure.w32
  make -j5 USE_HOSTTOOLS=no setup.exe
)}

if ! test -f db_dump.exe; then
  if ! curl "http://git.hohnstaedt.de/db_dump.exe" -o "db_dump.exe"; then
    echo db_dump.exe missing
    exit 1
  fi
fi

if ! test -f mingwm10.dll; then
  if ! zcat /usr/share/doc/mingw32-runtime/mingwm10.dll.gz >mingwm10.dll; then
    echo missing mingwm10.dll
    exit 1
  fi
fi

export INSTALL_DIR=`pwd`/install

do_openssl
do_libtool
do_OpenSC
do_XCA

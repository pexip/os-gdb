#!/bin/bash

# This script assumes that it will be called like:
#
#   SCRIPT --upstream-version VERSION
#
# It also assumes that it will be invoked from inside the source tree.
#
# It works with uscan v4.

set -ex
set -o pipefail

die()
{
    printf "E: %s\n" "$*"
    exit 1
}

UPSTREAM_VERSION="${2}"

TARBALL=$(realpath -e "../gdb_${UPSTREAM_VERSION}.orig.tar.xz")

TARDIR=$(dirname "${TARBALL}")
DFSG_TAR="${TARDIR}/gdb_${UPSTREAM_VERSION}.orig.tar.xz"
DOC_TAR="${TARDIR}/gdb-doc_${UPSTREAM_VERSION}.orig.tar.xz"

tmpdir=$(mktemp -d)
trap 'rm -rf "${tmpdir}"' EXIT ERR INT

cd "${tmpdir}"
mkdir src
cd src
tar xf "${TARBALL}"
cd ..

src=$(readlink -f "src/gdb-${UPSTREAM_VERSION}")
dest=$(readlink -f "gdb-${UPSTREAM_VERSION}")
destdoc=$(readlink -f "gdb-doc-${UPSTREAM_VERSION}")

[ ! -d "${src}" ] && die "Could not find source directory ${src}"

if [ -z "${dest}" ] || [ -e "${dest}" ]; then
  die "Could not create dest directory ${dest}"
fi

cp -a "${src}" "${dest}"
cp -a "${src}" "${destdoc}"

pushd "${dest}" > /dev/null

# All of the gdb manpages are GFDL'd now
find gdb -type f -name '*.[1-9]' -delete

# Almost all of the texinfo documentation is GFDL.  PSIM's is not, but
# we don't need that manual especially anyway.  Special care must be taken
# with observer.texi, which is necessary for the build process.  Remove
# all pregenerated info files, then replace all texinfo files with dummy
# versions.

find . -type f \( -name '*.info' -o -name '*.info-*' \) -delete
find . -type f -name '*.chm' -delete

find . -type f \( -name '*.texinfo' -o -name '*.texi' \) | \
    while read -r file; do
        if [ "$(basename "${file}")" = "observer.texi" ]; then
            sed -ne '/@c This/,/@c any later/p; /@deftype/p' "${src}/${file}" > "${file}"
            continue
        fi
        echo > "${file}"
    done

popd > /dev/null

tar --auto-compress -cf "${DFSG_TAR}" "gdb-${UPSTREAM_VERSION}"

pushd "${destdoc}" > /dev/null
find . -type f -name '*.chm' -delete
popd > /dev/null

tar --auto-compress -cf "${DOC_TAR}" "gdb-doc-${UPSTREAM_VERSION}"

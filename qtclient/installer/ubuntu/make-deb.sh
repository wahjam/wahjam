#!/bin/sh
# Build for Ubuntu and generate a .deb package.
#
# Usage: build-deb.sh wahjam|jammr [tag]

target=${1:-wahjam}
tag=${2:-HEAD}
version=$(git grep 'VERSION =' "$tag" -- qtclient/qtclient.pro | awk '{ print $4 }')
builddir="build/$target-client-$version"

cd "$(dirname $0)"

# Build image, if necessary
if [ $(podman images --noheading --filter reference="wahjam-builder" | wc -l) -eq 0 ]; then
    buildah bud --tag "wahjam-builder" Dockerfile
fi

rm -rf "$builddir"
mkdir -p "$builddir"
(cd ../../.. && git archive --format=tar "$tag") | tar -x -C "$builddir" -f -
ln -s qtclient/installer/ubuntu/debian "$builddir/"

podman run --rm -it \
	--user "$(id --user):$(id --group)" \
	--userns keep-id \
	--volume ./build:/usr/src/build:z \
	--workdir "/usr/src/$builddir" \
	wahjam-builder \
	dpkg-buildpackage -us -uc && \
rm -rf "$builddir"

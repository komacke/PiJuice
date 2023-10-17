#!/bin/bash

HERE="$(dirname "$0")"
THIS="$(basename "$0")"
cd "$HERE"
RPMBUILD_HOME=~/rpmbuild
PIJUICE_VERSION=$(PYTHONPATH=.. python3 -c "import pijuice; print(pijuice.__version__)")
RPMSPEC=pijuice-gui.spec

if [ ! -d $RPMBUILD_HOME ]; then
    echo "No rpmbuild folder found. Consider running 'rpmdev-setuptree'."
    exit 1
fi
if [ ! -f $RPMSPEC ]; then
    echo "No specfile named '$RPMSPEC' found."
    exit 1
fi

NAME=$(sed -n 's/^Name:\s\+\(.*\)/\1/p' $RPMSPEC)

rm -f $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz
tar -czf $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz --transform 's,^.,pijuice-gui-'$PIJUICE_VERSION',' -C .. . -C ../.. ./LICENSE
RETVAL=$?
if [ $RETVAL -ne 0 ]; then
    echo "Tar failed with exit code $RETVAL."
    exit $RETVAL
fi

echo "tarball created for version $PIJUICE_VERSION in rpmbuild."
echo "Try running 'rpmbuild -bb pijuice-gui.spec'."

#!/bin/bash

HERE="$(dirname "$0")"
THIS="$(basename "$0")"
cd "$HERE"

RPMSPEC=$(basename ./*.spec)
if [ ! -f $RPMSPEC ]; then
    echo "No specfile named '$RPMSPEC' found."
    exit 1
fi
RPMSPEC_BASE=${RPMSPEC%.spec}

[ -n "$COPR" ] && RPMBUILD_HOME="$COPR" || RPMBUILD_HOME=~/rpmbuild
if [ ! -d $RPMBUILD_HOME ]; then
    echo "No rpmbuild folder found. Consider running 'rpmdev-setuptree'."
    exit 1
fi

PIJUICE_VERSION=$(PYTHONPATH=.. python3 -c "import pijuice; print(pijuice.__version__)")
NAME=$(sed -n 's/^Name:\s\+\(.*\)/\1/p' $RPMSPEC)
sed -i 's/^\(Version:\s\+\)__version__\s*/\1'$PIJUICE_VERSION'/' $RPMSPEC

rm -f $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz
tar -czf $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz --transform 's,^.,'$RPMSPEC_BASE-$PIJUICE_VERSION',' -C .. . -C ../.. ./LICENSE
RETVAL=$?
sed -i 's/^\(Version:\s\+\).*/\1__version__/' $RPMSPEC
if [ $RETVAL -ne 0 ]; then
    echo "Tar failed with exit code $RETVAL."
    exit $RETVAL
fi

echo "tarball created for version $PIJUICE_VERSION in rpmbuild."
echo "Try running 'rpmbuild -bb $RPMSPEC'."

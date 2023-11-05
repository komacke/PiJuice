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
if ! grep -qs '^ExecStartPre=.*ds1307 0x68' ../data/pijuice.service; then
    sed -i '/^ExecStart=/iExecStartPre=+/bin/sh -c "[ ! -d /sys/module/rtc_ds1307 ] && echo ds1307 0x68 >/sys/class/i2c-adapter/i2c-__I2C_BUS__/new_device || :"' ../data/pijuice.service
fi

rm -f $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz
tar -czf $RPMBUILD_HOME/SOURCES/$NAME-$PIJUICE_VERSION.tgz --transform 's,^.,'$RPMSPEC_BASE-$PIJUICE_VERSION',' -C .. . -C ../.. ./LICENSE
RETVAL=$?
if [ $RETVAL -ne 0 ]; then
    echo "Tar failed with exit code $RETVAL."
    exit $RETVAL
fi

echo "Success! tarball created for version $PIJUICE_VERSION in rpmbuild."
echo "Try running 'rpmbuild -bb $RPMSPEC'."

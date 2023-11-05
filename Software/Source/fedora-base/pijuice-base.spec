Name:           pijuice-base
Version:        __version__
Release:        3%{?dist}
Summary:        Basic support for Pi-Supply's PiJuice HAT

License:        GPLv3+
URL:            https://github.com/komacke/PiJuice 
Source0:        %{name}-%{version}.tgz

BuildRequires:  python3, bash, python3-i2c-tools, systemd-rpm-macros
Requires:       i2c-tools, python3-i2c-tools, python3-tkinter, python3-urwid

%define  debug_package %{nil}


%description
This package contains the following python scripts:
 * The low level interface (pijuice.py)
 * The supporting service (pijuice_sys.py)
 * and the command line interface (pijuice_cli, pijuice_cli.py).

The dekstop applications are in the pijuice-gui package which depends on this one.


%prep
%autosetup


%pre
# Create user pijuice if it does not exist, only on fresh install
if [ $1 -eq 1 ]; then
    if id -u pijuice > /dev/null 2>&1; then
	    echo "Strange. User 'pijuice' already exists"
    else
        useradd --shell %{_sbindir}/nologin -r --home %{_sharedstatedir}/pijuice --comment "" pijuice
        usermod -a -G i2c pijuice
	    # Add default user (usually sudo user or guess) to the pijuice group
        [ -n "$SUDO_USER" ] && POWER_USER=$SUDO_USER || POWER_USER=$(id -un 1000)
        usermod -a -G pijuice $POWER_USER
    fi
fi


%preun
# only if uninstalling
if [ $1 -eq 0 ]; then
    systemctl --quiet disable pijuice.service
fi
%systemd_preun pijuice.service


%install
mkdir -p %{buildroot}%{_datadir}/pijuice/data
mkdir -p %{buildroot}%{_sharedstatedir}/pijuice
mkdir -p %{buildroot}%{_udevrulesdir}
mkdir -p %{buildroot}%{_sysconfdir}/sudoers.d
mkdir -p %{buildroot}%{_unitdir}
mkdir -p %{buildroot}%{_presetdir}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_tmpfilesdir}
mkdir -p %{buildroot}%{_libdir}/python%{__default_python3_version}/site-packages

cp -r data/firmware %{buildroot}%{_datadir}/pijuice/data
cp data/99-i2c.rules %{buildroot}%{_udevrulesdir}
cp data/98-local_i2c_group.rules %{buildroot}%{_udevrulesdir}
cp data/020_pijuice-nopasswd %{buildroot}%{_sysconfdir}/sudoers.d
cp data/pijuice.service %{buildroot}%{_unitdir}
cp data/98-pijuice.preset %{buildroot}%{_presetdir}
cp src/pijuice_sys.py %{buildroot}%{_bindir}
cp src/pijuice_cli.py %{buildroot}%{_bindir}
cp src/pijuice_log.py %{buildroot}%{_bindir}
cp data/pijuice.conf %{buildroot}%{_tmpfilesdir}
cp pijuice.py %{buildroot}%{_libdir}/python%{__default_python3_version}/site-packages

cp bin/pijuiceboot64 %{buildroot}%{_bindir}
cp bin/pijuice_cli64 %{buildroot}%{_bindir}
pushd %{buildroot}%{_bindir}
ln -s pijuiceboot64 pijuiceboot
ln -s pijuice_cli64 pijuice_cli
popd

echo "{\"system_task\":{\"enabled\": __SYSTEM_TASK_ENABLED__},\"board\":{\"general\":{\"i2c_bus\": __I2C_BUS__}}}" > %{buildroot}%{_sharedstatedir}/pijuice/pijuice_config.JSON


%post
unset I2C_BUS
# it's probably 3 so reverse to find it faster
for f in $(ls -d /sys/class/i2c-dev/* | sort -r); do
    TEMP_I2C_BUS=$(sed -n 's/^MINOR=\([[:digit:]]\)\+/\1/p' $f/uevent)
    # bang it 3 times fast to wake it up
    for i in {1..3}; do
        %{_sbindir}/i2cget -y $TEMP_I2C_BUS 0x14 >/dev/null 2>&1
        RETVAL=$?
        if [ $RETVAL -eq 0 ]; then
            I2C_BUS=$TEMP_I2C_BUS
            break 2
        fi
    done
done

if [ -n "$I2C_BUS" ]; then
    sed -i "s/__I2C_BUS__/$I2C_BUS/" %{_unitdir}/pijuice.service %{_sharedstatedir}/pijuice/pijuice_config.JSON*
    sed -i "s/__SYSTEM_TASK_ENABLED__/true/" %{_sharedstatedir}/pijuice/pijuice_config.JSON*
else
    echo "WARNING: no working i2c_bus found."
    sed -i "s/__I2C_BUS__/1/" %{_unitdir}/pijuice.service %{_sharedstatedir}/pijuice/pijuice_config.JSON*
    sed -i "s/__SYSTEM_TASK_ENABLED__/false/" %{_sharedstatedir}/pijuice/pijuice_config.JSON*
fi

udevadm trigger /dev/i2c-$I2C_BUS
%systemd_post pijuice.service
systemctl daemon-reload
if [ $1 -eq 1 ]; then
    # install
    systemctl start pijuice.service
elif [ $1 -eq 2 ]; then
    # upgrade
    systemctl try-restart pijuice.service
fi


%postun
# only if uninstalling
if [ $1 -eq 0 ]; then
    [ -n "$SUDO_USER" ] && POWER_USER=$SUDO_USER || POWER_USER=$(id -un 1000)
    usermod -r -G pijuice $POWER_USER
    userdel pijuice
    rm -Rf %{_sharedstatedir}/pijuice
fi

%files
%defattr(644,root,root,-)
%license LICENSE
%{_datadir}/pijuice
%{_sysconfdir}/sudoers.d/020_pijuice-nopasswd
%{_unitdir}/pijuice.service
%{_presetdir}/98-pijuice.preset
%{_tmpfilesdir}/pijuice.conf
%{_udevrulesdir}/98-local_i2c_group.rules
%{_udevrulesdir}/99-i2c.rules
%{_libdir}/python%{__default_python3_version}/site-packages/__pycache__/pijuice.cpython-312.opt-1.pyc
%{_libdir}/python%{__default_python3_version}/site-packages/__pycache__/pijuice.cpython-312.pyc
%{_libdir}/python%{__default_python3_version}/site-packages/pijuice.py
%{_bindir}/pijuice_log.py
%{_bindir}/pijuice_cli
%{_bindir}/pijuiceboot

%attr(755,root,root) %{_bindir}/pijuiceboot64
%attr(755,root,root) %{_bindir}/pijuice_sys.py

%attr(644,pijuice,pijuice) %{_bindir}/pijuice_cli.py

%attr(4755,pijuice,pijuice) %{_bindir}/pijuice_cli64

%attr(700,pijuice,pijuice) %dir %{_sharedstatedir}/pijuice
%config(noreplace) %attr(600,pijuice,pijuice) %{_sharedstatedir}/pijuice/pijuice_config.JSON


%changelog
* Mon Oct 16 2023 Dave Koberstein <davek@komacke.com>
- initial spec file 

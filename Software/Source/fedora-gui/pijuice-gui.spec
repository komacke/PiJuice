Name:           pijuice-gui
Version:        __version__
Release:        1%{?dist}
Summary:        The desktop applications for the Pi-Supply PiJuice HAT

License:        GPLv3+
URL:            https://github.com/komacke/PiJuice 
Source0:        %{name}-%{version}.tgz

BuildRequires:  python3, bash, python3-i2c-tools, systemd-rpm-macros
Requires:       pijuice-base

%define  debug_package %{nil}


%description
This package contains the desktop configuration utility (pijuice_gui.py)
and the tray application (pijuice_tray.py) indicating the battery charge level.
It also makes an entry in the Preferences pointing to the configuration utility.

The basic PiJuice support is in the pijuice-base package.


%prep
%autosetup


%pre


%post
# only if upgrading
if [ $1 -eq 2 ]; then
    if [ -f %{_rundir}/pijuice/pijuice_tray.pid ]; then
        pkill -F %{_rundir}/pijuice/pijuice_tray.pid
        sudo -u pijuice /usr/bin/pijuice_tray.py &
    fi
fi


%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_datadir}/pijuice/data
mkdir -p %{buildroot}%{_sysconfdir}/X11/xinit/xinitrc.d
mkdir -p %{buildroot}%{_sysconfdir}/xdg/autostart/

cp src/pijuice_tray.py %{buildroot}%{_bindir}
cp src/pijuice_gui.py %{buildroot}%{_bindir}
cp bin/pijuice_gui64 %{buildroot}%{_bindir}
cp data/pijuice-gui.desktop %{buildroot}%{_datadir}/applications
cp -r data/images %{buildroot}%{_datadir}/pijuice/data
cp data/36x11-pijuice_xhost %{buildroot}%{_sysconfdir}/X11/xinit/xinitrc.d
cp data/pijuice-tray.desktop %{buildroot}%{_sysconfdir}/xdg/autostart/
pushd %{buildroot}%{_bindir}
ln -sf pijuice_gui64 pijuice_gui
popd


%preun
# only if uninstalling
if [ $1 -eq 0 ]; then
    if [ -f %{_rundir}/pijuice/pijuice_tray.pid ]; then
        pkill -F %{_rundir}/pijuice/pijuice_tray.pid
        rm %{_rundir}/pijuice/pijuice_tray.pid
    fi
fi


%postun


%files
%defattr(644,root,root,-)
%{_datadir}/pijuice/data/images
%{_datadir}/applications/pijuice-gui.desktop
%{_sysconfdir}/xdg/autostart/pijuice-tray.desktop
%{_bindir}/pijuice_gui

%attr(644,pijuice,pijuice) %{_bindir}/pijuice_gui.py

%attr(4755,pijuice,pijuice) %{_bindir}/pijuice_gui64

%attr(755,root,root) %{_bindir}/pijuice_tray.py
%attr(755,root,root) %{_sysconfdir}/X11/xinit/xinitrc.d/36x11-pijuice_xhost


%changelog
* Mon Oct 16 2023 Dave Koberstein <davek@komacke.com>
- initial spec file 

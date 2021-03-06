# enable debug features such as control environment variables
# WARNING! do not use for production builds as it will break security
%define debug_build 0
%define efl 0

%if %{debug_build} == 1
%define extra_config_options1 --enable-gtk-doc --enable-debug
%endif

%if %{efl} == 1
%define extra_config_options1 --enable-examples
%endif


Name:    tlm
Summary: Login manager for Tizen
Version: 1.0.3
Release: 0
Group:   System/Service
License: LGPL-2.1+
URL:     https://github.com/01org/tlm
Source:     %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Requires(post): /sbin/ldconfig
Requires(post): systemd
Requires(postun): /sbin/ldconfig
Requires(postun): systemd
Requires: gumd
Requires: libsystemd
BuildRequires: pkgconfig(glib-2.0) >= 2.30
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(gmodule-2.0)
BuildRequires: pkgconfig(libgum)
BuildRequires: pam-devel
%if %{debug_build} == 1
BuildRequires: gtk-doc
%endif
%if %{efl} == 1
BuildRequires: pkgconfig(elementary)
%endif

%description
TLM is a daemon that handles user logins in a multi-user, multi-seat system by
authenticating the users through PAM, and setting up, launching, and tracking
user sessions.

%package devel
Summary:    Dev files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development files for %{name}.


%package doc
Summary:    Doc files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description doc
Documentation files for %{name}.


%if "%{profile}" != "ivi"

%package config-common
Summary:    Configuration files for common-profile
Group:      System/Service
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-config

%description config-common
Tiny Login Manager configuration files for common-profile.

%else

%package config-ivi-singleseat
Summary:    Configuration files for ivi-profile with single seat
Group:	    System/Service
Requires:   %{name} = %{version}-%{release}
Requires:   weekeyboard
Provides:   %{name}-config
Conflicts:  %{name}-config-ivi-modello, %{name}-config-ivi-singleseat-ico
Conflicts:  %{name}-config-ivi-multiseat, %{name}-config-ivi-vtc1010
Conflicts:  ico-uxf-weston-plugin

%description config-ivi-singleseat
Generic Tiny Login Manager configuration files for ivi-profile with
single seat.


%package config-ivi-singleseat-modello
Summary:    Configuration files for ivi-profile with single seat for modello
Group:      System/Service
Requires:   %{name} = %{version}-%{release}
Requires:   Modello-Installer-xwalk
Requires:   weekeyboard
Provides:   %{name}-config
Conflicts:  %{name}-config-ivi-singleseat, %{name}-config-ivi-singleseat-ico
Conflicts:  %{name}-config-ivi-multiseat, %{name}-config-ivi-vtc1010
Conflicts:  ico-uxf-weston-plugin

%description config-ivi-singleseat-modello
Generic Tiny Login Manager configuration files for ivi-profile with
single seat for modello.


%package config-ivi-singleseat-ico
Summary:    Configuration files for ivi-profile with single seat for ico
Group:      System/Service
Requires:   %{name} = %{version}-%{release}
Requires:   ico-uxf-homescreen
Provides:   %{name}-config
Conflicts:  %{name}-config-ivi-singleseat, %{name}-config-ivi-singleseat-modello
Conflicts:  %{name}-config-ivi-multiseat, %{name}-config-ivi-vtc1010

%description config-ivi-singleseat-ico
Generic Tiny Login Manager configuration files for ivi-profile with
single seat for ico.


%package config-ivi-multiseat
Summary:    Configuration files for ivi-profile with multi seat
Group:	    System/Service
Requires:   %{name} = %{version}-%{release}
Requires:   weekeyboard
Provides:   %{name}-config
Conflicts:  %{name}-config-ivi-singleseat, %{name}-config-ivi-singleseat-modello
Conflicts:  %{name}-config-ivi-singleseat-ico, %{name}-config-ivi-vtc1010
Conflicts:  ico-uxf-weston-plugin

%description config-ivi-multiseat
Generic Tiny Login Manager configuration files for ivi-profile with
multi seat.


%package config-ivi-vtc1010
Summary:    Configuration files for ivi-profile on VTC-1010
Group:      System/Service
Requires:   weekeyboard
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-config
Conflicts:  %{name}-config-ivi-singleseat, %{name}-config-ivi-singleseat-modello
Conflicts:  %{name}-config-ivi-singleseat-ico, %{name}-config-ivi-multiseat
Conflicts:  ico-uxf-weston-plugin

%description config-ivi-vtc1010
Tiny Login Manager configuration files for ivi-profile on VTC-1010 hardware.

%endif


%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} .


%build
%if %{debug_build} == 1
./autogen.sh
%endif
%reconfigure --enable-gum \
             %{?extra_config_options1:%extra_config_options1} \
             %{?extra_config_options2:%extra_config_options2}
%__make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%make_install
rm -f %{buildroot}%{_sysconfdir}/tlm.conf
install -m 755 -d %{buildroot}%{_unitdir}
install -m 644 data/tlm.service %{buildroot}%{_unitdir}
install -m 755 -d %{buildroot}%{_sysconfdir}/pam.d
install -m 644 data/tizen/tlm-login %{buildroot}%{_sysconfdir}/pam.d/
install -m 644 data/tizen/tlm-default-login %{buildroot}%{_sysconfdir}/pam.d/
install -m 644 data/tizen/tlm-system-login %{buildroot}%{_sysconfdir}/pam.d/
install -m 755 -d %{buildroot}%{_sysconfdir}/session.d
%if "%{profile}" == "ivi"
install -m 755 -d %{buildroot}%{_sysconfdir}/xdg/weston
install -m 644 data/tizen-ivi/etc/tlm*.conf %{buildroot}%{_sysconfdir}
install -m 755 data/tizen-ivi/etc/session.d/* %{buildroot}%{_sysconfdir}/session.d/
install -m 644 data/tizen-ivi/etc/xdg/weston/*.ini %{buildroot}%{_sysconfdir}/xdg/weston/
install -m 755 -d %{buildroot}%{_sysconfdir}/udev/rules.d
install -m 644 data/tizen-ivi/10-multiseat-vtc1010.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
install -m 755 -d %{buildroot}%{_sysconfdir}/profile.d
install -m 644 data/tizen-ivi/etc/profile.d/* %{buildroot}%{_sysconfdir}/profile.d/
%else
install -m 644 data/tizen-common/etc/tlm.conf %{buildroot}%{_sysconfdir}
install -m 755 data/tizen-common/etc/session.d/* %{buildroot}%{_sysconfdir}/session.d/
%endif


%post
/sbin/ldconfig


%postun -p /sbin/ldconfig


%if "%{profile}" != "ivi"

%post config-common
systemctl enable tlm
systemctl daemon-reload

%preun config-common
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-common
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi

%else

%post config-ivi-singleseat
if [ ! -e /etc/tlm.conf ] || [ -h /etc/tlm.conf ]; then
	ln -s -f /etc/tlm-singleseat.conf /etc/tlm.conf
fi
systemctl enable tlm
systemctl daemon-reload

%preun config-ivi-singleseat
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-ivi-singleseat
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi


%post config-ivi-singleseat-modello
if [ ! -e /etc/tlm.conf ] || [ -h /etc/tlm.conf ]; then
	ln -s -f /etc/tlm-singleseat-modello.conf /etc/tlm.conf
fi
systemctl enable tlm
systemctl daemon-reload

%preun config-ivi-singleseat-modello
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-ivi-singleseat-modello
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi


%post config-ivi-singleseat-ico
if [ ! -e /etc/tlm.conf ] || [ -h /etc/tlm.conf ]; then
	ln -s -f /etc/tlm-singleseat-ico.conf /etc/tlm.conf
fi
systemctl enable tlm
systemctl daemon-reload

%preun config-ivi-singleseat-ico
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-ivi-singleseat-ico
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi


%post config-ivi-multiseat
if [ ! -e /etc/tlm.conf ] || [ -h /etc/tlm.conf ]; then
	ln -s -f /etc/tlm-multiseat.conf /etc/tlm.conf
fi
systemctl enable tlm
systemctl daemon-reload

%preun config-ivi-multiseat
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-ivi-multiseat
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi


%post config-ivi-vtc1010
if [ ! -e /etc/tlm.conf ] || [ -h /etc/tlm.conf ]; then
	ln -s -f /etc/tlm-vtc1010.conf /etc/tlm.conf
fi
systemctl enable tlm
systemctl daemon-reload

%preun config-ivi-vtc1010
if [ $1 == 0 ]; then
	systemctl disable tlm
	systemctl daemon-reload
fi

%postun config-ivi-vtc1010
if [ -h /etc/tlm.conf ] && [ $1 == 0 ]; then
	rm -f /etc/tlm.conf
fi

%endif


%files
%defattr(-,root,root,-)
%manifest %{name}.manifest
%license COPYING
%doc AUTHORS NEWS README
%{_bindir}/%{name}
%{_bindir}/%{name}-sessiond
%{_bindir}/%{name}-client
%{_bindir}/%{name}-launcher
%{_libdir}/lib%{name}*.so.*
%{_libdir}/%{name}/plugins/*.so*
%{_unitdir}/tlm.service
%config %{_sysconfdir}/pam.d/tlm-login
%config %{_sysconfdir}/pam.d/tlm-default-login
%config %{_sysconfdir}/pam.d/tlm-system-login


%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/*.h
%{_libdir}/lib%{name}*.so
%{_libdir}/pkgconfig/%{name}.pc
%if %{efl} == 1
%{_bindir}/tlm-ui
%endif


%files doc
%defattr(-,root,root,-)
%{_datadir}/gtk-doc/html/tlm/*


%if "%{profile}" != "ivi"

%files config-common
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm.conf
%config(noreplace) %{_sysconfdir}/session.d/*

%else

%files config-ivi-singleseat
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm-singleseat.conf
%config(noreplace) %{_sysconfdir}/session.d/genivi-session-singleseat
%config(noreplace) %{_sysconfdir}/session.d/user-session
%config(noreplace) %{_sysconfdir}/session.d/user-session-launch-script
%config(noreplace) %{_sysconfdir}/session.d/user-session-weston
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-genivi.ini
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-user.ini
%config(noreplace) %{_sysconfdir}/profile.d/weston-env-ivi.sh


%files config-ivi-singleseat-modello
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm-singleseat-modello.conf
%config(noreplace) %{_sysconfdir}/session.d/genivi-session-modello
%config(noreplace) %{_sysconfdir}/session.d/user-session-modello
%config(noreplace) %{_sysconfdir}/session.d/user-session-modello-launch-script
%config(noreplace) %{_sysconfdir}/session.d/user-session-modello-weston
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-genivi-modello.ini
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-user-modello.ini
%config(noreplace) %{_sysconfdir}/profile.d/weston-env-ivi.sh


%files config-ivi-singleseat-ico
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm-singleseat-ico.conf
%config(noreplace) %{_sysconfdir}/session.d/genivi-session-singleseat
%config(noreplace) %{_sysconfdir}/session.d/user-session-ico
%config(noreplace) %{_sysconfdir}/session.d/user-session-ico-launch-script
%config(noreplace) %{_sysconfdir}/session.d/user-session-ico-weston
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-genivi.ini
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-user.ini
%config(noreplace) %{_sysconfdir}/profile.d/weston-env-ivi.sh


%files config-ivi-multiseat
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm-multiseat.conf
%config(noreplace) %{_sysconfdir}/session.d/genivi-session-multiseat
%config(noreplace) %{_sysconfdir}/session.d/user-session
%config(noreplace) %{_sysconfdir}/session.d/user-session-launch-script
%config(noreplace) %{_sysconfdir}/session.d/user-session-weston
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-genivi.ini
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-user.ini
%config(noreplace) %{_sysconfdir}/profile.d/weston-env-ivi.sh


%files config-ivi-vtc1010
%defattr(-,root,root,-)
%manifest %{name}.manifest
%config(noreplace) %{_sysconfdir}/tlm-vtc1010.conf
%config(noreplace) %{_sysconfdir}/session.d/genivi-session-vtc1010
%config(noreplace) %{_sysconfdir}/session.d/user-session
%config(noreplace) %{_sysconfdir}/session.d/user-session-launch-script
%config(noreplace) %{_sysconfdir}/session.d/user-session-weston
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-genivi-vtc1010.ini
%config(noreplace) %{_sysconfdir}/xdg/weston/weston-user.ini
%config(noreplace) %{_sysconfdir}/udev/rules.d/*
%config(noreplace) %{_sysconfdir}/profile.d/weston-env-ivi.sh

%endif


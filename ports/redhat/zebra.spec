%define 	with_snmp	0
%define		with_vtysh	1
%define		with_ospf_te	1
%define		with_nssa	1
%define		with_opaque_lsa 1
%define		with_tcp_zebra	0
%define		with_vtysh	1
%define		with_pam	1
%define		with_ipv6	1
%define		with_multipath	32
%define		_sysconfdir	/etc/zebra

Summary: Routing daemon
Name:		zebra
Version:	0.93b
Release:	2002111101
License:	GPL
Group: System Environment/Daemons
Source0:	ftp://ftp.zebra.org/pub/zebra/%{name}-%{version}.tar.gz
Source1:        zebra.init
Source2:        bgpd.init
Source3:        ospf6d.init
Source4:        ospfd.init
Source5:        ripd.init
Source6:        ripngd.init
Source8:	zebra.pam
Source9:	zebra.logrotate
Source10:	zebra.mpls-docs.tar.gz
Patch0:		zebra-bgpd-hash.patch
Patch1:		zebra-ptp.patch
Patch2:		zebra-linkstate.patch
Patch3:		zebra-ospfd-ptmp.patch
Patch4:		zebra-ospfd-misc.patch
Patch5:		zebra-ospfd-olsa.patch
Patch7:		zebra-vtysh-write-config.patch
Patch8:		zebra-ospfd-md5auth-seqnum.patch
Patch9:		zebra-ospfd-oi_write_q.patch
Patch10:	zebra-ripv1-netmask.patch
Patch11:	zebra-ospfd-md5-buffer-fix.patch
Patch12:	zebra-multi.patch
URL:		http://www.zebra.org/
%if %with_snmp
BuildRequires:	ucd-snmp-devel
Prereq:		ucd-snmp
%endif
%if %with_vtysh
BuildRequires:	readline readline-devel ncurses ncuses-devel
Prereq:		readline ncurses
%endif
BuildRequires:	texinfo tetex autoconf openssl-devel pam-devel patch
# Initscripts > 5.60 is required for IPv6 support
Prereq:		openssl ncurses readline initscripts pam
Prereq:		/sbin/install-info
Provides:	routingdaemon
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
Obsoletes:	bird gated mrt

%description
GNU Zebra is a free software that manages TCP/IP based routing
protocol. It takes multi-server and multi-thread approach to resolve
the current complexity of the Internet.

GNU Zebra supports BGP4, BGP4+, OSPFv2, OSPFv3, RIPv1, RIPv2, and RIPng.

GNU Zebra is intended to be used as a Route Server and a Route
Reflector. It is not a toolkit, it provides full routing power under
a new architecture. GNU Zebra is unique in design in that it has a
process for each protocol.

%prep
%setup  -q
%patch0 -p1 -b .bgphash
%patch1 -p0 -b .ptp
%patch2 -p1 -b .linkstate
%patch3 -p1 -b .ospf-ptmp
%patch4 -p1 -b .ospf-misc
%patch5 -p1 -b .ospf-olsa
%patch7 -p1 -b .vtysh-write
%patch8 -p2 -b .ospf-md5auth-seqnum
%patch9 -p0 -b .ospfd-oi_write_q
%patch10 -p0 -b .ripd-netmask
%patch11 -p0 -b .ospfd-md5-buffer
%patch12 -p0 -b .zebra-multi
%{__tar} -zxf %{SOURCE10}

%build
%configure \
	--with-cflags="-O2" \
	--enable-netlink \
%if %with_ipv6
	--enable-ipv6 \
%endif
%if %with_snmp
	--enable-snmp \
%endif
%if %with_multipath
	--enable-multipath=%with_multipath \
%endif
%if %with_tcp_zebra
	--enable-tcp-zebra \
%endif
%if %with_nssa
	--enable-nssa \
%endif
%if %with_opaque_lsa
	--enable-opaque-lsa \
%endif
%if %with_ospf_te
	--enable-ospf-te \
%endif
%if %with_vtysh
	--enable-vtysh \
%endif
%if %with_pam
	--with-libpam
%endif

pushd vtysh
make %{?_smp_mflags} rebuild
popd

make %{?_smp_mflags} MAKEINFO="makeinfo --no-split"

pushd doc
texi2html zebra.texi
popd

%install
rm -rf $RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT/etc/{rc.d/init.d,sysconfig,logrotate.d,pam.d} \
	$RPM_BUILD_ROOT/var/log/zebra $RPM_BUILD_ROOT%{_infodir}

make install \
	DESTDIR=$RPM_BUILD_ROOT

install %{SOURCE1} $RPM_BUILD_ROOT/etc/rc.d/init.d/zebra
install %{SOURCE2} $RPM_BUILD_ROOT/etc/rc.d/init.d/bgpd
%if %with_ipv6
install %{SOURCE3} $RPM_BUILD_ROOT/etc/rc.d/init.d/ospf6d
%endif
install %{SOURCE4} $RPM_BUILD_ROOT/etc/rc.d/init.d/ospfd
install %{SOURCE5} $RPM_BUILD_ROOT/etc/rc.d/init.d/ripd
%if %with_ipv6
install %{SOURCE6} $RPM_BUILD_ROOT/etc/rc.d/init.d/ripngd
%endif
install -m644 %{SOURCE8} $RPM_BUILD_ROOT/etc/pam.d/zebra
install -m644 %{SOURCE9} $RPM_BUILD_ROOT/etc/logrotate.d/zebra

%post
# zebra_spec_add_service <sercice name> <port/proto> <comment>
# e.g. zebra_spec_add_service zebrasrv 2600/tcp "zebra service"

zebra_spec_add_service ()
{
  # Add port /etc/services entry if it isn't already there 
  if [ -f /etc/services ] && ! grep -q "^$1[^a-zA-Z0-9]" /etc/services ; then
    echo "$1		$2			# $3"  >> /etc/services
  fi
}

zebra_spec_add_service zebrasrv 2600/tcp "zebra service"
zebra_spec_add_service zebra    2601/tcp "zebra vty"
zebra_spec_add_service ripd     2602/tcp "RIPd vty"
%if %with_ipv6
zebra_spec_add_service ripngd   2603/tcp "RIPngd vty"
%endif
zebra_spec_add_service ospfd    2604/tcp "OSPFd vty"
zebra_spec_add_service bgpd     2605/tcp "BGPd vty"
%if %with_ipv6
zebra_spec_add_service ospf6d   2606/tcp "OSPF6d vty"
%endif

/sbin/chkconfig --add zebra 
/sbin/chkconfig --add ripd
%if %with_ipv6
/sbin/chkconfig --add ripngd
%endif
/sbin/chkconfig --add ospfd
%if %with_ipv6
/sbin/chkconfig --add ospf6d
%endif
/sbin/chkconfig --add bgpd

/sbin/install-info %{_infodir}/zebra.info.gz %{_infodir}/dir

# Create dummy files if they don't exist so basic functions can be used.
if [ ! -e %{_sysconfdir}/zebra.conf ]; then
	echo "hostname `hostname`" > %{_sysconfdir}/zebra.conf
	chmod 640 %{_sysconfdir}/zebra.conf
fi
if [ ! -e %{_sysconfdir}/vtysh.conf ]; then
	touch %{_sysconfdir}/vtysh.conf
	chmod 640 %{_sysconfdir}/vtysh.conf
fi

%postun
if [ "$1" -ge  "1" ]; then
	/etc/rc.d/init.d/zebra  condrestart >/dev/null 2>&1
	/etc/rc.d/init.d/ripd   condrestart >/dev/null 2>&1
%if %with_ipv6
	/etc/rc.d/init.d/ripngd condrestart >/dev/null 2>&1
%endif
	/etc/rc.d/init.d/ospfd  condrestart >/dev/null 2>&1
%if %with_ipv6
	/etc/rc.d/init.d/ospf6d condrestart >/dev/null 2>&1
%endif
	/etc/rc.d/init.d/bgpd   condrestart >/dev/null 2>&1
fi
/sbin/install-info --delete %{_infodir}/zebra.info.gz %{_infodir}/dir

%preun
if [ "$1" = "0" ]; then
        /sbin/chkconfig --del zebra
	/sbin/chkconfig --del ripd
%if %with_ipv6
	/sbin/chkconfig --del ripngd
%endif
	/sbin/chkconfig --del ospfd
%if %with_ipv6
	/sbin/chkconfig --del ospf6d
%endif
	/sbin/chkconfig --del bgpd
fi

%clean
#rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc */*.sample* doc/zebra.html tools AUTHORS COPYING
%doc ChangeLog INSTALL NEWS README REPORTING-BUGS SERVICES TODO
%dir %attr(750,root,root) %{_sysconfdir}
%dir %attr(750,root,root) /var/log/zebra
%dir %attr(755,root,root) /usr/share/info
%{_infodir}/*info*
%{_mandir}/man*/*
%{_sbindir}/*
%if %with_vtysh
%{_bindir}/*
%endif
%config /etc/zebra/*
%config /etc/rc.d/init.d/*
%config(noreplace) /etc/pam.d/zebra
%config(noreplace) %attr(640,root,root) /etc/logrotate.d/*

%changelog
* Sat Dec 28 2002 Alexander Hoogerhuis <alexh@ihatent.com>
- Added conditionals for building with(out) IPv6, vtysh, RIP, BGP
- Fixed up some build requirements (patch)
- Added conditional build requirements for vtysh / snmp
- Added conditional to %files for %_bindir depending on vtysh

* Mon Nov 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add Greg Troxel's md5 buffer copy/dup fix
- add RIPv1 fix
- add Frank's multicast flag fix

* Wed Oct 09 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- timestamped crypt_seqnum patch
- oi->on_write_q fix

* Mon Sep 30 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add vtysh 'write-config (integrated|daemon)' patch
- always 'make rebuild' in vtysh/ to catch new commands

* Fri Sep 13 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93b

* Wed Sep 11 2002 Paul Jakma <paulj@alphyra.ie>
- update to latest CVS
- add "/sbin/ip route flush proto zebra" to zebra RH init on startup

* Sat Aug 24 2002 Paul Jakma <paulj@alphyra.ie>
- update to current CVS
- add OSPF point to multipoint patch
- add OSPF bugfixes
- add BGP hash optimisation patch

* Fri Jun 14 2002 Paul Jakma <paulj@alphyra.ie>
- update to 0.93-pre1 / CVS
- add link state detection support
- add generic PtP and RFC3021 support
- various bug fixes

* Thu Aug 09 2001 Elliot Lee <sopwith@redhat.com> 0.91a-6
- Fix bug #51336

* Wed Aug  1 2001 Trond Eivind Glomsr�d <teg@redhat.com> 0.91a-5
- Use generic initscript strings instead of initscript specific
  ( "Starting foo: " -> "Starting $prog:" )

* Fri Jul 27 2001 Elliot Lee <sopwith@redhat.com> 0.91a-4
- Bump the release when rebuilding into the dist.

* Tue Feb  6 2001 Tim Powers <timp@redhat.com>
- built for Powertools

* Sun Feb  4 2001 Pekka Savola <pekkas@netcore.fi> 
- Hacked up from PLD Linux 0.90-1, Mandrake 0.90-1mdk and one from zebra.org.
- Update to 0.91a
- Very heavy modifications to init.d/*, .spec, pam, i18n, logrotate, etc.
- Should be quite Red Hat'isque now.
Name:      bacnet-stack
Summary:   bacnet-stack
Version:   1.0.0
Release:   0
Group:     System/Libraries
License:   GPL-2.0-or-later with GCC-exception-2.0 and MIT and Apache-2.0 and BSD-3-Clause and CC-PDDC and GPL-2.0-or-later
Source0:   %{name}-%{version}.%{release}.tar.gz

BuildRequires: cmake
BuildRequires: hash-signer

%define tizen_sign 1
%define tizen_sign_base %{_prefix}/apps/%{name}
%define tizen_sign_level platform
%define tizen_author_sign 1
%define tizen_dist_sign 1

%define _prefix /usr
%define _pkgdir %{_prefix}/apps/%{name}
%define _manifestdir %{TZ_SYS_RO_PACKAGES}

%define logtag "bacnet-stack"

%description
This package contains components licensed under multiple open source licenses:
- GPL-2.0-or-later with GCC-exception-2.0 (core BACnet library)
- MIT (most object and service implementations)
- Apache-2.0 (networking components)
- BSD-3-Clause (utility functions)
- CC-PDDC (public domain components)
- GPL-2.0-or-later (some core components)

%package devel
Summary:  Development files for %{name} library
Group:    System/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the header files and development libraries necessary to develop applications that use %{name} library.

%prep
%setup -q

%build

cmake \
	-DCMAKE_INSTALL_PREFIX=%{_prefix} \
	-DCMAKE_INSTALL_BINDIR=%{_pkgdir}/bin \
	-DCMAKE_INSTALL_LIBDIR=%{_libdir} \
	-DCMAKE_INSTALL_INCLUDEDIR=%{_includedir}/%{name} \
	-DBUILD_SHARED_LIBS=ON \
	-DBACNET_STACK_BUILD_APPS=ON \
	-DBACNET_STACK_INSTALL_APPS=ON \
	-DTIZEN_BUILD=ON \
	-DBAC_ROUTING=ON \
	-DMULTI_DEVICE=ON \
	-DBUILD_SHARED_LIBS=ON \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make %{?jobs:-j%jobs}

%install
%make_install


%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%preun

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest
%{_libdir}/lib%{name}.so*
%{_pkgdir}/bin/*
%{_pkgdir}/author-signature.xml
%{_pkgdir}/signature1.xml

%files devel
%manifest %{name}.manifest
%{_includedir}/%{name}/*
%{_libdir}/cmake/%{name}/*
%{_libdir}/pkgconfig/%{name}.pc

Name: liquidmaya
Summary: Renderman translator for MAYA
Version: 1.8
Release: Pixie
License: MPL
Group: Applications
Source: %{name}-%{version}.tar.gz


BuildRoot: %{_tmppath}/build-root-%{name}
Packager: Cedric PAILLE
Distribution: suse 10
Prefix: /usr
Url: http://sourceforge.net/projects/liquidmaya
Provides: Liquid rendering toolkit for PIXIE



%description
Liquid it a tool for translating Maya scenes into another renderer's scene description.
Currently just RenderMan is supported.
For more information please see the project homepage.

%prep
rm -rf $RPM_BUILD_ROOT 
mkdir $RPM_BUILD_ROOT

%setup -q

%build
export LIQRMAN=pixie
export PIXIEHOME=/opt/pixie
export MAYA_LOCATION=/usr/aw/maya
export PATH=$PATH:/opt/pixie/bin
export CPP=g++334
cd shaders
sh compile.sh
cd ..
make release

%install
export LIQRMAN=pixie
export PIXIEHOME=/opt/pixie
export MAYA_LOCATION=/usr/aw/maya
export PATH=$PATH:/opt/pixie/bin
mkdir -p $RPM_BUILD_ROOT/usr/aw/maya/bin/plug-ins
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,0755)
%dir /usr/aw/maya/bin/plug-ins
/usr/aw/maya/bin/plug-ins/liquid.so
%dir /usr/share/liquid
%dir /usr/share/liquid/mel
/usr/share/liquid/mel/*.mel
%dir /usr/share/liquid/icons
/usr/share/liquid/icons/*.xpm
/usr/share/liquid/icons/*.iff
/usr/share/liquid/icons/*.jpg
%dir /usr/share/liquid/renderers
/usr/share/liquid/renderers/*.lg
%dir /usr/share/liquid/displayDrivers
%dir /usr/share/liquid/displayDrivers/Pixie
/usr/share/liquid/displayDrivers/Pixie/*.so
%dir /usr/share/liquid/shaders
/usr/share/liquid/shaders/*.sdr
%dir /usr/share/liquid/shaders/src
/usr/share/liquid/shaders/src/*.sl

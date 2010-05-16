@echo off 

SET tmpdir=tmp
if exist %tmpdir% rmdir /s /q %tmpdir%
mkdir %tmpdir%
cd %tmpdir%

SET LIQUIDHOME=c:/tools/liquidmaya
SET VSINSTALLDIR="C:\Program Files (x86)\Microsoft Visual Studio 8"

SET GEN="NMake Makefiles"
rem SET GEN="Visual Studio 8 2005"
rem SET GEN="Visual Studio 8 2005 Win64"

SET ZLIB_INCLUDE_DIRS=C:/tools/zlib/include

rem SET ARCH=
rem SET ZLIB_LIBRARIES=C:/tools/zlib/lib/zlib.lib
rem call %VSINSTALLDIR%\VC\vcvarsall.bat x86


SET ARCH=-x64
SET ZLIB_LIBRARIES=C:/tools/zlib64/static_x64/zlibstat.lib
call %VSINSTALLDIR%\VC\vcvarsall.bat amd64


rem if exist override.cmd call override.cmd
echo Building ribLib for Liquid %ARCH% 
echo using %GEN% ...

cmake -G %GEN% -D CMAKE_INSTALL_PREFIX:PATH=../ribLib ../ribLib

nmake
nmake install
cd ..


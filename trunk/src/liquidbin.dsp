# Microsoft Developer Studio Project File - Name="liquidbin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=liquidbin - Win32 PRMan 10 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "liquidbin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "liquidbin.mak" CFG="liquidbin - Win32 PRMan 10 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "liquidbin - Win32 PRMan 11 Release" (based on "Win32 (x86) Application")
!MESSAGE "liquidbin - Win32 PRMan 11 Debug" (based on "Win32 (x86) Application")
!MESSAGE "liquidbin - Win32 PRMan 10 Release" (based on "Win32 (x86) Application")
!MESSAGE "liquidbin - Win32 PRMan 10 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "liquidbin - Win32 PRMan 11 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin/win32/Release"
# PROP Intermediate_Dir "../bin/win32/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libprmutil.lib SHELL32.LIB libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Release/liquid.exe" /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "liquidbin - Win32 PRMan 11 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin/win32/Debug"
# PROP Intermediate_Dir "../bin/win32/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libprmutil.lib libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Debug/liquid.exe" /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "liquidbin - Win32 PRMan 10 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "liquidbin___Win32_PRMan_10_Release"
# PROP BASE Intermediate_Dir "liquidbin___Win32_PRMan_10_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin/win32/Release"
# PROP Intermediate_Dir "../bin/win32/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib libprmutil.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib /nologo /subsystem:console /machine:I386 /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 SHELL32.LIB libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Release/liquid.exe" /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "liquidbin - Win32 PRMan 10 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "liquidbin___Win32_PRMan_10_Debug"
# PROP BASE Intermediate_Dir "liquidbin___Win32_PRMan_10_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin/win32/Debug"
# PROP Intermediate_Dir "../bin/win32/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /D "_BOOL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib libprmutil.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 SHELL32.LIB libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Debug/liquid.exe" /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "liquidbin - Win32 PRMan 11 Release"
# Name "liquidbin - Win32 PRMan 11 Debug"
# Name "liquidbin - Win32 PRMan 10 Release"
# Name "liquidbin - Win32 PRMan 10 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\liqAqsisRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\liqDelightRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\liqEntropyRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\liqGetSloInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\liqGlobalHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\liqPrmanRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\liqProcessLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\liqRibParticleData.cpp
# End Source File
# Begin Source File

SOURCE=.\liqRibSurfaceData.cpp
# End Source File
# Begin Source File

SOURCE=.\liqShader.cpp
# End Source File
# Begin Source File

SOURCE=.\liqTokenPointer.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidBin.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibCoordData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibGenData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibHT.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibLightData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibLocatorData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibMeshData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibNode.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibNuCurveData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibObj.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibSubdivisionData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibTranslator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

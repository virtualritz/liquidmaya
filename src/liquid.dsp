# Microsoft Developer Studio Project File - Name="liquid" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=liquid - Win32 PRMan 11 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "liquid.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "liquid.mak" CFG="liquid - Win32 PRMan 11 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "liquid - Win32 PRMan 10 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "liquid - Win32 PRMan 10 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "liquid - Win32 PRMan 11 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "liquid - Win32 PRMan 11 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "liquid - Win32 PRMan 10 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "PRMAN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /dll /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Release/liquid.mll" /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "liquid - Win32 PRMan 10 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Debug/liquid.mll" /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"

!ELSEIF  "$(CFG)" == "liquid - Win32 PRMan 11 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "liquid___Win32_PRMan_11_Release"
# PROP BASE Intermediate_Dir "liquid___Win32_PRMan_11_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin/win32/Release"
# PROP Intermediate_Dir "../bin/win32/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "PRMAN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "PRMAN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib /nologo /dll /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Release/liquid.mll" /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 libprmutil.lib libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /dll /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Release/liquid.mll" /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "liquid - Win32 PRMan 11 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "liquid___Win32_PRMan_11_Debug"
# PROP BASE Intermediate_Dir "liquid___Win32_PRMan_11_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin/win32/Debug"
# PROP Intermediate_Dir "../bin/win32/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "$(MAYA_LOCATION)/include" /I "$(RMANTREE)/include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIQUID_EXPORTS" /D "_BOOL" /D ulong="unsigned long" /D uint="unsigned int" /D "PRMAN" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Debug/liquid.mll" /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"
# ADD LINK32 libprmutil.lib libsloargs.lib librib.lib libtarget.lib libzip.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib liblkm.lib ws2_32.lib netapi32.lib shell32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libmmt.lib" /nodefaultlib:"libircmt.lib" /out:"../bin/win32/Debug/liquid.mll" /pdbtype:sept /libpath:"$(MAYA_LOCATION)/lib" /libpath:"$(RMANTREE)/lib"

!ENDIF 

# Begin Target

# Name "liquid - Win32 PRMan 10 Release"
# Name "liquid - Win32 PRMan 10 Debug"
# Name "liquid - Win32 PRMan 11 Release"
# Name "liquid - Win32 PRMan 11 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\liqRibParticleData.cpp
# End Source File
# Begin Source File

SOURCE=.\liqShader.cpp
# End Source File
# Begin Source File

SOURCE=.\liqTokenPointer.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidAttachPrefAttribute.cpp
# End Source File
# Begin Source File

SOURCE=.\liqGetAttr.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidGetSloInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidGlobalHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidPlug.cpp
# End Source File
# Begin Source File

SOURCE=.\liqPreviewShader.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidProcessLauncher.cpp
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

SOURCE=.\liquidRibSurfaceData.cpp
# End Source File
# Begin Source File

SOURCE=.\liquidRibTranslator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\liqRibParticleData.h
# End Source File
# Begin Source File

SOURCE=..\include\liqShader.h
# End Source File
# Begin Source File

SOURCE=..\include\liqTokenPointer.h
# End Source File
# Begin Source File

SOURCE=..\include\liquid.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidAttachPrefAttribute.h
# End Source File
# Begin Source File

SOURCE=..\include\liqGetAttr.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidGetSloInfo.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidGlobalHelpers.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidMemory.h
# End Source File
# Begin Source File

SOURCE=..\include\liqPreviewShader.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidProcessLauncher.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibCoordData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRIBGen.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibGenData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibHT.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibItHT.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibLightData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibLocatorData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibMeshData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibNode.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibNuCurveData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibObj.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRIBStatus.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibSubdivisionData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibSurfaceData.h
# End Source File
# Begin Source File

SOURCE=..\include\liquidRibTranslator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "MEL Scripts"

# PROP Default_Filter "*.mel"
# Begin Source File

SOURCE=..\mel\getMultiListerSelection.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidAttachParams.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidAttrWindow.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidConvMTOR.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidDelayedReadArchive.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidFileBrowser.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidFluid.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidGlobals.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidHelpers.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidMenuHelp.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidParamDialogWindow.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidPrefAttribute.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidReadArchive.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidReAttachParams.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidRibBox.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidRibGen.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidShaders.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidShutdown.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidStartup.mel
# End Source File
# Begin Source File

SOURCE=..\mel\liquidSubD.mel
# End Source File
# End Group
# End Target
# End Project

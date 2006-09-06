; Liquid installer

Name "LIQUID FOR MAYA"
OutFile "LiquidForMaya7.exe"

XPStyle on

AddBrandingImage left 100

; Sets the font of the installer
SetFont "Comic Sans MS" 8

LicenseText "License page"
LicenseData "mpl.rtf"
InstallDir "$PROGRAMFILES\Alias\Maya7.0\bin\plug-ins"
DirText "Please select the Maya's plugin folder ..."

Var /GLOBAL Mayahome
Var /GLOBAL Mayaplugdir
; Macros

!macro BIMAGE IMAGE PARMS
	Push $0
	GetTempFileName $0
	File /oname=$0 "${IMAGE}"
	SetBrandingImage ${PARMS} $0
	Delete $0
	Pop $0
!macroend

; Pages

Page directory dirFun
Page license licenseFunc
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles


; Sections

Section ""
	StrCpy $Mayaplugdir $INSTDIR
	ClearErrors
	IfFileExists $DOCUMENTS\maya\7.0\Maya.env.liquidbackup 0 +2
	Goto envfound
	MessageBox MB_YESNO "Do you want the installer try to setup your Maya.env file?" IDYES yestry IDNO envfound
	yestry:
	MessageBox MB_YESNO "Do you want to make a copy of the Maya.env file?" IDNO nobackup
	CopyFiles $DOCUMENTS\maya\7.0\Maya.env $DOCUMENTS\maya\7.0\Maya.env.liquidbackup
	nobackup:
	IfFileExists $DOCUMENTS\maya\7.0\Maya.env +2 0
	Goto envnotfound
	FileOpen $0 $DOCUMENTS\maya\7.0\Maya.env a
	IfErrors envnotfound
	FileSeek $0 0 END
	FileWrite $0 "$\nLIQUIDHOME = $PROGRAMFILES\Liquid"
	FileWrite $0 "$\nXBMLANGPATH = $PROGRAMFILES\Liquid\icons\$\n"
	FileWrite $0 "$\nMAYA_PLUG_IN_PATH = $PROGRAMFILES\Liquid\plugin$\n"
	FileClose $0
	Goto envfound
	envnotfound:
	MessageBox MB_OK "Maya.env file is not found, check your MAYA installation, or try manually"
	MessageBox MB_OK "Open Maya.env file and add these lines:$\nLIQUIDHOME = $PROGRAMFILES\Liquid$\nXBMLANGPATH = $PROGRAMFILES\Liquid\icons\"
	Quit
	envfound:
    CreateDirectory "$PROGRAMFILES\Liquid"
	SetOutPath "$PROGRAMFILES\Liquid\mel"
	CreateDirectory "$PROGRAMFILES\Liquid\mel"
	File "..\mel\*.mel"
	File "..\mel\*.png"
	SetOutPath "$PROGRAMFILES\Liquid\renderers"
	CreateDirectory "$PROGRAMFILES\Liquid\renderers"
	File "..\renderers\*.lg"
	SetOutPath "$PROGRAMFILES\Liquid\icons"
	CreateDirectory "$PROGRAMFILES\Liquid\icons"
	File "..\icons\*.*"
	SetOutPath "$PROGRAMFILES\Liquid\shaders"
	CreateDirectory "$PROGRAMFILES\Liquid\shaders"
	File "..\shaders\compile.cmd"
	SetOutPath "$PROGRAMFILES\Liquid\shaders\src"
	CreateDirectory "$PROGRAMFILES\Liquid\shaders\src"
	File "..\shaders\src\*.*"
	SetOutPath "$PROGRAMFILES\Liquid\previewRibFiles"
	CreateDirectory "$PROGRAMFILES\Liquid\previewRibFiles"
	File "..\previewRibFiles\*.rib"
	CreateDirectory "$PROGRAMFILES\Liquid\plugin"
	SetOutPath "$PROGRAMFILES\Liquid\plugin"
	File "..\bin\Generic\win32\release\liquid.mll"
	File "..\bin\zlib1.dll"
    CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers"
	CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers\Pixie"
	SetOutPath "$PROGRAMFILES\Liquid\displayDrivers\Pixie"
	File "..\bin\Pixie\win32\release\liqmaya.dll"
	CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers\3Delight"
	SetOutPath "$PROGRAMFILES\Liquid\displayDrivers\3Delight"
	File "..\bin\3Delight\win32\release\liqmaya.dpy"
	CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers\Aqsis"
	SetOutPath "$PROGRAMFILES\Liquid\displayDrivers\Aqsis"
	File "..\bin\Aqsis\win32\release\liqmaya.dll"
	CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers\Air"
	SetOutPath "$PROGRAMFILES\Liquid\displayDrivers\Air"
	File "..\bin\Air\win32\release\d_liqmaya.so"
	SetOutPath "$PROGRAMFILES\Liquid\scripts"
	File "..\scripts\*.*"
	MessageBox MB_YESNO "Do you want to compile shaders?" IDNO noshaders
	SetOutPath "$PROGRAMFILES\Liquid\shaders"
	Exec "$PROGRAMFILES\Liquid\shaders\compile.cmd"
	noshaders:
	WriteUninstaller $PROGRAMFILES\Liquid\uninst.exe
	MessageBox MB_OK "Installation complete, if the modules don't load, get a copy of zlib1.dll and put it in your system32 directory"
SectionEnd

Section "Uninstall"
	ClearErrors
	MessageBox MB_YESNO "Uninstall Liquid for MAYA?" IDNO end
	Delete $PROGRAMFILES\liquid\uninst.exe
	RMDir /r "$PROGRAMFILES\Liquid"
	MessageBox MB_OK "Liquid deleted, please delete liquid.mll yourself in the Maya's plugins folder."
	end:
	SectionEnd

Function licenseFunc
	!insertmacro BIMAGE "liquidlarge.bmp" /RESIZETOFIT
FunctionEnd

Function dirFun
	ClearErrors
	ReadEnvStr $Mayahome MAYA_LOCATION
	IfErrors next
		MessageBox MB_YESNO "MAYA_LOCATION found in your environment variables, using it?" IDNO next
		StrCpy $INSTDIR "$Mayahome\bin\plug-ins"
		Abort
	next:
	IfFileExists "C:\Program files\Alias\Maya7.0\bin\plug-ins" 0 notfound
		MessageBox MB_YESNO "Maya found at C:\Program files\Alias\Maya7.0, use this one?" IDNO notfound
		StrCpy $INSTDIR "C:\Program files\Alias\Maya7.0\bin\plug-ins"
		Abort
	 notfound:
FunctionEnd

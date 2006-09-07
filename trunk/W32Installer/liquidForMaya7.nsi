; Liquid installer

Name "LIQUID FOR MAYA"
OutFile "LiquidForMaya7.exe"

XPStyle on

AddBrandingImage left 100

; Sets the font of the installer
SetFont "Comic Sans MS" 8

LicenseText "License page"
LicenseData "mpl.rtf"

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

Page license licenseFunc
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles


; Sections

Section ""
	ClearErrors
	IfFileExists $DOCUMENTS\maya\7.0\Maya.env.liquidbackup 0 foundlbu
	MessageBox MB_OK "Installer have detected an older installation, please desinstall and replace the Maya.env.backup file with Maya.env and delete Maya.env.backup.$\nInstallation aborted, sorry."
	Quit
	foundlbu:
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
	FileWrite $0 "MAYA_PLUG_IN_PATH = $PROGRAMFILES\Liquid\plugin$\n"
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
	ClearErrors
	FileOpen $0 "$PROGRAMFILES\Liquid\displayDrivers\Aqsis\displays.ini" w
	IfErrors erroraqsisdpy
	FileWrite $0 "liqmaya			liqmaya.dll"
	FileClose $0
	erroraqsisdpy:
	SetOutPath "$PROGRAMFILES\Liquid\scripts"
	File "..\scripts\*.*"
	MessageBox MB_YESNO "Do you want to compile shaders?" IDNO noshaders
	SetOutPath "$PROGRAMFILES\Liquid\shaders"
	Exec "$PROGRAMFILES\Liquid\shaders\compile.cmd"
	noshaders:
	WriteUninstaller $PROGRAMFILES\Liquid\uninst.exe
SectionEnd

Section "Uninstall"
	ClearErrors
	MessageBox MB_YESNO "Uninstall Liquid for MAYA?" IDNO end
	Delete $PROGRAMFILES\liquid\uninst.exe
	RMDir /r "$PROGRAMFILES\Liquid"
	MessageBox MB_YESNO "Do you want to restore your old Maya.env file?" IDNO end
	ClearErrors
	IfFileExists $DOCUMENTS\maya\7.0\Maya.env.liquidbackup 0 end
	Rename $DOCUMENTS\maya\7.0\Maya.env $DOCUMENTS\maya\7.0\Maya.env.bak
	CopyFiles $DOCUMENTS\maya\7.0\Maya.env.liquidbackup $DOCUMENTS\maya\7.0\Maya.env
	Delete $DOCUMENTS\maya\7.0\Maya.env.liquidbackup
	end:
SectionEnd

Function licenseFunc
	!insertmacro BIMAGE "liquidlarge.bmp" /RESIZETOFIT
FunctionEnd

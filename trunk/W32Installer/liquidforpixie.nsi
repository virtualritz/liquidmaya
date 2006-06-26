; Liquid installer

Name "LIQUID FOR MAYA"
OutFile "LiquidMayaForPixie.exe"

XPStyle on

AddBrandingImage left 100

; Sets the font of the installer
SetFont "Comic Sans MS" 8

LicenseText "License page"
LicenseData "mpl.rtf"
InstallDir "$PROGRAMFILES\Liquid"

Var /GLOBAL Mayahome
Var /GLOBAL RendererHome
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
	SetOutPath "$PROGRAMFILES\Liquid\renderers"
	CreateDirectory "$PROGRAMFILES\Liquid\renderers"
	File "..\renderers\*.lg"
	SetOutPath "$PROGRAMFILES\Liquid\icons"
	CreateDirectory "$PROGRAMFILES\Liquid\icons"
	File "..\icons\*.*"
	SetOutPath "$PROGRAMFILES\Liquid\shaders"
	CreateDirectory "$PROGRAMFILES\Liquid\shaders"
	File "..\shaders\*.sdr"
	SetOutPath "$PROGRAMFILES\Liquid\previewRibFiles"
	CreateDirectory "$PROGRAMFILES\Liquid\previewRibFiles"
	File "..\previewRibFiles\*.rib"
	SetOutPath "$Mayahome\bin\plug-ins"
	File "..\bin\Pixie\win32\release\liquid.mll"
        CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers"
	CreateDirectory "$PROGRAMFILES\Liquid\displayDrivers\Pixie"
	SetOutPath "$PROGRAMFILES\Liquid\displayDrivers\Pixie"
	File "..\bin\Pixie\win32\release\liqmaya.dll"
    	SetOutPath "$PROGRAMFILES\Liquid\scripts"
	File "..\scripts\*.*"
	WriteUninstaller $PROGRAMFILES\Liquid\uninst.exe
SectionEnd

Section "Uninstall"
	ClearErrors
	ReadEnvStr $Mayahome MAYA_LOCATION
	IfErrors abortMaya
	MessageBox MB_YESNO "Uninstall Liquid for MAYA?" IDYES continue
	Quit
	continue:
	goto theend
	abortMaya:
	StrCpy $Mayahome "$PROGRAMFILES\alias"
    theend:
	Delete $PROGRAMFILES\liquid\uninst.exe
	RMDir /r "$PROGRAMFILES\Liquid"
	Delete $Mayahome\bin\plug-ins\liquid.mll
SectionEnd

Function licenseFunc
	!insertmacro BIMAGE "liquidlarge.bmp" /RESIZETOFIT
	ClearErrors
	ReadEnvStr $Mayahome MAYA_LOCATION
	IfErrors abortMaya
	ReadEnvStr $RendererHome PIXIEHOME
	IfErrors abortRenderer
	goto theend
	abortRenderer:
	MessageBox MB_OK "Pixie not found cannot install.$\nCheck your Pixie installation"
	Quit
	abortMaya:
	MessageBox MB_YESNO "MAYA_LOCATION not found, Assume $PROGRAMFILES\alias ?" IDNO noassume
	StrCpy $Mayahome "$PROGRAMFILES\alias"
	goto theend
	noassume:
	Quit
	theend:
FunctionEnd

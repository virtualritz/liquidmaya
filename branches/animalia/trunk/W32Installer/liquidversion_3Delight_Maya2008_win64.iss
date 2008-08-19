[Setup]
AppName=Liquid for 3Delight
AppVersion=2.3.0
AppVerName=Liquid for Maya 2008 64bit
OutputBaseFilename=Liquid_2.3.0_3Delight_Maya2008_win64_Setup
[Messages]
BeveledLabel=Liquid for 3Delight Setup
[Files]
Source: ..\bin\3Delight\win64\liquid.mll; DestDir: {app}\bin\maya2008\win64\3Delight;
Source: ..\bin\3Delight\win64\liquid.exe; DestDir: {app}\bin\maya2008\win64\3Delight;
Source: ..\displayDrivers\3Delight\liqmaya.dpy; DestDir: {app}\display;

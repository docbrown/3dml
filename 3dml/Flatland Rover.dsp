# Microsoft Developer Studio Project File - Name="Flatland Rover" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FLATLAND ROVER - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Flatland Rover.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Flatland Rover.mak" CFG="FLATLAND ROVER - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Flatland Rover - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Flatland Rover - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Flatland Rover - Win32 Release No Taskbar" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Flatland Rover"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Flatland Rover - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob0 /I "amd" /I "Real" /I "collision" /I "jpeg" /I "unzip" /I "plugin" /I "a3d" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "TASK_BAR" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 amd3d.lib ddraw.lib dsound.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.dll" /libpath:"amd"
# SUBTRACT LINK32 /profile /map /debug

!ELSEIF  "$(CFG)" == "Flatland Rover - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Ob0 /I "." /I "amd" /I "real" /I "jpeg" /I "unzip" /I "collision" /I "plugin" /I "a3d" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "TASK_BAR" /D "TRACE" /D "VERBOSE" /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 amd3d.lib ddraw.lib dsound.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.pdb" /map:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.dll" /libpath:"amd"
# SUBTRACT LINK32 /profile /pdb:none

!ELSEIF  "$(CFG)" == "Flatland Rover - Win32 Release No Taskbar"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Flatlan1"
# PROP BASE Intermediate_Dir "Flatlan1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Flatlan1"
# PROP Intermediate_Dir "Flatlan1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /Ob0 /I "Include" /I "amd" /I "Real" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "TASK_BAR" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob0 /I "Include" /I "amd" /I "Real" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 amd3d.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.dll" /libpath:"Lib" /libpath:"amd"
# SUBTRACT BASE LINK32 /profile /map /debug
# ADD LINK32 amd3d.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"c:\Program Files\Netscape\Communicator\Program\Plugins\NPRover.dll" /libpath:"Lib" /libpath:"amd"
# SUBTRACT LINK32 /profile /map /debug

!ENDIF 

# Begin Target

# Name "Flatland Rover - Win32 Release"
# Name "Flatland Rover - Win32 Debug"
# Name "Flatland Rover - Win32 Release No Taskbar"
# Begin Group "Source files"

# PROP Default_Filter "cpp"
# Begin Group "Collision source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Collision\Col.cpp
# End Source File
# Begin Source File

SOURCE=.\Collision\Collision.cpp
# End Source File
# Begin Source File

SOURCE=.\Collision\Mat.cpp
# End Source File
# Begin Source File

SOURCE=.\Collision\Maths.cpp
# End Source File
# Begin Source File

SOURCE=.\Collision\Vec.cpp
# End Source File
# End Group
# Begin Group "Plugin API source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Plugin\Npwin.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes.cpp
# End Source File
# Begin Source File

SOURCE=.\Fileio.cpp
# End Source File
# Begin Source File

SOURCE=.\Image.cpp
# End Source File
# Begin Source File

SOURCE=.\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\Plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\real.cpp
# End Source File
# Begin Source File

SOURCE=.\Render.cpp
# End Source File
# Begin Source File

SOURCE=.\Spans.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Group "Collision header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Collision\Col.h
# End Source File
# Begin Source File

SOURCE=.\Collision\Collision.h
# End Source File
# Begin Source File

SOURCE=.\Collision\Mat.h
# End Source File
# Begin Source File

SOURCE=.\Collision\Maths.h
# End Source File
# Begin Source File

SOURCE=.\Collision\Vec.h
# End Source File
# End Group
# Begin Group "Plugin API header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Plugin\jni.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\jni_md.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\jri.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\jri_md.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\jritypes.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\npapi.h
# End Source File
# Begin Source File

SOURCE=.\Plugin\npupp.h
# End Source File
# End Group
# Begin Group "JPEG library header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\Jerror.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\Jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\Jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpeg\Jpeglib.h
# End Source File
# End Group
# Begin Group "Unzip library header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\unzip\Unzip.h
# End Source File
# Begin Source File

SOURCE=.\unzip\Zconf.h
# End Source File
# Begin Source File

SOURCE=.\unzip\Zlib.h
# End Source File
# End Group
# Begin Group "A3D API header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\A3d\ia3dapi.h
# End Source File
# Begin Source File

SOURCE=.\A3d\Ia3dutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Classes.h
# End Source File
# Begin Source File

SOURCE=.\Fileio.h
# End Source File
# Begin Source File

SOURCE=.\Image.h
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\Main.h
# End Source File
# Begin Source File

SOURCE=.\memory.h
# End Source File
# Begin Source File

SOURCE=.\Parser.h
# End Source File
# Begin Source File

SOURCE=.\Platform.h
# End Source File
# Begin Source File

SOURCE=.\Plugin.h
# End Source File
# Begin Source File

SOURCE=.\Real.h
# End Source File
# Begin Source File

SOURCE=.\Render.h
# End Source File
# Begin Source File

SOURCE=.\Spans.h
# End Source File
# Begin Source File

SOURCE=.\tags.h
# End Source File
# Begin Source File

SOURCE=.\Tokens.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# End Group
# Begin Group "Resource files"

# PROP Default_Filter "cur,unk,def"
# Begin Source File

SOURCE=.\Bitmaps\A3D.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_e.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_n.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_ne.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_nw.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_s.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_se.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_sw.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Arrow_w.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Builder0.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Builder1.unk
# End Source File
# Begin Source File

SOURCE=.\Exports.def
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Hand.cur
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\History0.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\History1.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Left.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Light0.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Light1.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Logo0.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Logo1.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Options0.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Options1.unk
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resources.rc
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Right.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Splash.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Title_bg.unk
# End Source File
# Begin Source File

SOURCE=.\Bitmaps\Title_end.unk
# End Source File
# End Group
# Begin Group "Static libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\A3d\ia3dutil.lib
# End Source File
# Begin Source File

SOURCE=.\jpeg\Jpeg.lib
# End Source File
# Begin Source File

SOURCE=.\unzip\Unzip.lib
# End Source File
# Begin Source File

SOURCE=.\Lib\Amstrmid.Lib
# End Source File
# Begin Source File

SOURCE=.\Lib\StrmBase.Lib
# End Source File
# Begin Source File

SOURCE=.\Lib\Quartz.Lib
# End Source File
# End Group
# End Target
# End Project

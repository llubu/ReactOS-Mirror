<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="main" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MAIN}" installbase="system32" installname="main.cpl" unicode="yes" crt="msvcrt">
	<importlibrary definition="main.spec" />
	<include base="main">.</include>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>devmgr</library>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>gdi32</library>
	<file>keyboard.c</file>
	<file>main.c</file>
	<file>mouse.c</file>
	<file>main.rc</file>
	<pch>main.h</pch>
</module>

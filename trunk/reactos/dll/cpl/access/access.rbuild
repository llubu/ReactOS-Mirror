<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="access" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_ACCESS}"  installbase="system32" installname="access.cpl" unicode="yes">
	<importlibrary definition="access.spec" />
	<include base="access">.</include>
	<library>gdi32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<file>access.c</file>
	<file>display.c</file>
	<file>general.c</file>
	<file>keyboard.c</file>
	<file>mouse.c</file>
	<file>sound.c</file>
	<file>access.rc</file>
	<pch>access.h</pch>
</module>

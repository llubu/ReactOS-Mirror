<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="logoff" type="win32cui" installbase="system32" installname="logoff.exe">
	<include base="logoff">.</include>
	<library>advapi32</library>
	<library>user32</library>
	<file>misc.c</file>
	<file>logoff.c</file>
	<file>logoff.rc</file>
	<pch>precomp.h</pch>
</module>

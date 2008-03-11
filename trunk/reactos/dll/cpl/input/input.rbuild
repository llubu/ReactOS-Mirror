<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="input" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_INPUT}" installbase="system32" installname="input.cpl" unicode="yes">
	<importlibrary definition="input.def" />
	<include base="input">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<file>input.c</file>
	<file>settings.c</file>
	<file>keysettings.c</file>
	<file>add.c</file>
	<file>changekeyseq.c</file>
	<file>inputlangprop.c</file>
	<file>input.rc</file>
</module>

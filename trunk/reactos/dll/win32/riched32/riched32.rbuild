<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="riched32" type="win32dll" baseaddress="${BASEADDRESS_RICHED32}" installbase="system32" installname="riched32.dll" allowwarnings="true">
	<importlibrary definition="riched32.spec" />
	<include base="riched32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>richedit.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>riched20</library>
	<library>user32</library>
	<library>ntdll</library>
</module>
</group>

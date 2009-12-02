<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rasapi32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_RASAPI32}" installbase="system32" installname="rasapi32.dll" allowwarnings="true">
	<importlibrary definition="rasapi32.spec" />
	<include base="rasapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>rasapi.c</file>
	<library>wine</library>
	<library>ntdll</library>
</module>
</group>

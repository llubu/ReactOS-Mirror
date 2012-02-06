<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sxs" type="win32dll" baseaddress="${BASEADDRESS_SXS}" installbase="system32" installname="sxs.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="sxs.spec" />
	<include base="sxs">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>cache.c</file>
	<file>sxs.c</file>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>wine</library>
	<library>ntdll</library>
</module>

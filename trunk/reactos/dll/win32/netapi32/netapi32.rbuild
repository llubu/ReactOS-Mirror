<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="netapi32" type="win32dll" baseaddress="${BASEADDRESS_NETAPI32}" installbase="system32" installname="netapi32.dll" allowwarnings="true">
	<importlibrary definition="netapi32.spec.def" />
	<include base="netapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>access.c</file>
	<file>apibuf.c</file>
	<file>browsr.c</file>
	<file>ds.c</file>
	<file>local_group.c</file>
	<file>nbcmdqueue.c</file>
	<file>nbnamecache.c</file>
	<file>nbt.c</file>
	<file>netapi32.c</file>
	<file>netbios.c</file>
	<file>share.c</file>
	<file>wksta.c</file>
	<file>netapi32.spec</file>
	<library>wine</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="hlink" type="win32dll" baseaddress="${BASEADDRESS_HLINK}" installbase="system32" installname="hlink.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="hlink.spec.def" />
	<include base="hlink">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>browse_ctx.c</file>
	<file>extserv.c</file>
	<file>hlink_main.c</file>
	<file>link.c</file>
	<file>hlink.spec</file>
	<library>wine</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>urlmon</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>

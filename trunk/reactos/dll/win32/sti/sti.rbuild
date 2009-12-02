<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="sti" type="win32dll" baseaddress="${BASEADDRESS_STI}" installbase="system32" installname="sti.dll" allowwarnings="true" entrypoint="0">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="sti.spec" />
	<include base="sti">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<file>regsvr.c</file>
	<file>sti_main.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>ntdll</library>
</module>

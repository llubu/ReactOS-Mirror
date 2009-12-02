<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rsaenh" type="win32dll" baseaddress="${BASEADDRESS_RSAENH}" installbase="system32" installname="rsaenh.dll" allowwarnings="true" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rsaenh.spec" />
	<include base="rsaenh">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>aes.c</file>
	<file>des.c</file>
	<file>handle.c</file>
	<file>implglue.c</file>
	<file>md2.c</file>
	<file>mpi.c</file>
	<file>rc2.c</file>
	<file>rc4.c</file>
	<file>rsa.c</file>
	<file>rsaenh.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>crypt32</library>
	<library>advapi32</library>
	<library>ntdll</library>
</module>
</group>

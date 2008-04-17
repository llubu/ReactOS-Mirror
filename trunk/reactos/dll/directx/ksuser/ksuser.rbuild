<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ksuser" type="win32dll" baseaddress="${BASEADDRESS_KSUSER}" installbase="system32" installname="ksuser.dll" allowwarnings="true">
	<importlibrary definition="ksuser.def" />
	<include base="ksuser">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>advapi32</library>	
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ksuser.c</file>
	<file>ksuser.rc</file>
</module>

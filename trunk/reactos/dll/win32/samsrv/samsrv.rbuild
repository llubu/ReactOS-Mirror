<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="samsrv" type="win32dll" baseaddress="${BASEADDRESS_SAMSRV}" entrypoint="0" installbase="system32" installname="samsrv.dll" unicode="yes">
	<importlibrary definition="samsrv.def" />
	<include base="samsrv">.</include>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>samsrv.c</file>
	<file>samsrv.rc</file>
</module>

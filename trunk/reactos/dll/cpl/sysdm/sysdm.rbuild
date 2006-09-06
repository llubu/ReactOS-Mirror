<module name="sysdm" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_SYSDM}" installbase="system32" installname="sysdm.cpl" usewrc="false">
	<importlibrary definition="sysdm.def" />
	<include base="sysdm">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>netapi32</library>
	<library>ntdll</library>
	<file>advanced.c</file>
	<file>computer.c</file>
	<file>environment.c</file>
	<file>general.c</file>
	<file>hardware.c</file>
	<file>sysdm.c</file>
	<file>userprofile.c</file>
	<file>virtmem.c</file>
	<file>sysdm.rc</file>
</module>

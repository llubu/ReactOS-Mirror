<module name="usrmgr" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_USRMGR}" installbase="system32" installname="usrmgr.cpl">
	<importlibrary definition="usrmgr.def" />
	<include base="usrmgr">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
<!--	<define name="WINVER">0x501</define> -->
<!--	<define name="_WIN32" /> -->
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>netapi32</library>
	<library>msvcrt</library>
	<file>extra.c</file>
	<file>groups.c</file>
	<file>users.c</file>
	<file>usrmgr.c</file>
	<file>usrmgr.rc</file>
</module>

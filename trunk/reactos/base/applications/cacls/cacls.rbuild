<module name="cacls" type="win32cui" installbase="system32" installname="cacls.exe">
	<include base="cacls">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<file>cacls.c</file>
	<file>lang/cacls.rc</file>
	<pch>precomp.h</pch>
</module>

<module name="hdwwiz" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_HDWWIZ}" installbase="system32" installname="hdwwiz.cpl">
	<importlibrary definition="hdwwiz.def" />
	<include base="hdwwiz">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>setupapi</library>
	<file>hdwwiz.c</file>
	<file>hdwwiz.rc</file>
</module>

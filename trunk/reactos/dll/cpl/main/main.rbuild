<module name="main" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MAIN}" installbase="system32" installname="main.cpl">
	<importlibrary definition="main.def" />
	<include base="main">.</include>
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
	<library>devmgr</library>
	<library>comdlg32</library>
	<library>gdi32</library>
	<file>keyboard.c</file>
	<file>main.c</file>
	<file>mouse.c</file>
	<file>main.rc</file>
</module>

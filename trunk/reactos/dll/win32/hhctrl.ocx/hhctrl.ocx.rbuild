<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="hhctrl" type="win32ocx" baseaddress="${BASEADDRESS_HHCTRL}" installbase="system32" installname="hhctrl.ocx" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="hhctrl.ocx.spec" />
	<include base="hhctrl">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS" root="intermediate">include/reactos</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<dependency>wineheaders</dependency>
	<file>chm.c</file>
	<file>content.c</file>
	<file>help.c</file>
	<file>hhctrl.c</file>
	<file>index.c</file>
	<file>regsvr.c</file>
	<file>search.c</file>
	<file>stream.c</file>
	<file>webbrowser.c</file>
	<file>hhctrl.rc</file>
	<pch>hhctrl.h</pch>
	<library>wine</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>

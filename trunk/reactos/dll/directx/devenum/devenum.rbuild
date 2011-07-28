<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="devenum" type="win32dll" baseaddress="${BASEADDRESS_DEVENUM}" installbase="system32" installname="devenum.dll" unicode="yes">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="devenum.spec" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<include base="devenum">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<library>strmiids</library>
	<library>user32</library>
	<library>avicap32</library>
	<file>devenum.rc</file>
	<file>createdevenum.c</file>
	<file>devenum_main.c</file>
	<file>factory.c</file>
	<file>mediacatenum.c</file>
	<file>parsedisplayname.c</file>
	<pch>devenum_private.h</pch>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cryptui" type="win32dll" baseaddress="${BASEADDRESS_CRYPTUI}" installbase="system32" installname="cryptui.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="cryptui.spec" />
	<include base="cryptui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WINE" />
	<file>main.c</file>
	<file>cryptui.rc</file>
	<file>cryptui_De.rc</file>
	<file>cryptui_En.rc</file>
	<file>cryptui_Fr.rc</file>
	<file>cryptui_Ko.rc</file>
	<file>cryptui_Nl.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>ole32</library>
	<library>crypt32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>urlmon</library>
	<library>wintrust</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>ntdll</library>
</module>
</group>

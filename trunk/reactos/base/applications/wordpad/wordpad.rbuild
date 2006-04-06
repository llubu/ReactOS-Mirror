<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe">
		<include base="wordpad">.</include>
		<define name="UNICODE" />
		<define name="_UNICODE" />		
		<define name="__USE_W32API" />
		<define name="_WIN32_IE">0x0600</define>
		<define name="_WIN32_WINNT">0x0501</define>
		<library>kernel32</library>
		<library>user32</library>
		<library>gdi32</library>
		<library>comdlg32</library>
		<library>advapi32</library>
		<library>shell32</library>
		<library>comctl32</library>
		<compilationunit name="unit.c">
			<file>dialogs.c</file>
			<file>editwnd.c</file>
			<file>mainwnd.c</file>
			<file>misc.c</file>
			<file>opensave.c</file>
			<file>wordpad.c</file>
		</compilationunit>
	  <file>wordpad.rc</file>
		<pch>precomp.h</pch>
	</module>
</rbuild>
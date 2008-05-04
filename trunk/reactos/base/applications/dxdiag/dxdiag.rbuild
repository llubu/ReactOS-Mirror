<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dxdiag" type="win32gui" installbase="system32" installname="dxdiag.exe">
	<include base="dxdiag">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>version</library>
	<library>dinput8</library>
	<library>setupapi</library>
	<library>dxguid</library>
	<library>dsound</library>
	<library>ddraw</library>
	<library>version</library>
	<library>gdi32</library>
	<library>winmm</library>
	<file>system.c</file>
	<file>display.c</file>
	<file>sound.c</file>
	<file>music.c</file>
	<file>input.c</file>
	<file>network.c</file>
	<file>help.c</file>
	<file>dxdiag.c</file>
	<file>dxdiag.rc</file>
	<file>ddtest.c</file>
	<pch>precomp.h</pch>
</module>

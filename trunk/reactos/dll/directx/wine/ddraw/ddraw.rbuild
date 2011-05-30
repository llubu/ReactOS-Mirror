<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ddraw" type="win32dll" installbase="system32" installname="ddraw.dll" allowwarnings ="true" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="ddraw.spec" />
	<include base="ddraw">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="USE_WIN32_OPENGL" />
	<compilerflag compilerset="msc">/FIwine/typeof.h</compilerflag>

	<library>advapi32</library>
	<library>dxguid</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>pseh</library>
	<library>user32</library>
	<library>uuid</library>
	<library>wine</library>
	<library>wined3d</library>

	<file>clipper.c</file>
	<file>ddraw.c</file>
	<file>device.c</file>
	<file>executebuffer.c</file>
	<file>light.c</file>
	<file>main.c</file>
	<file>material.c</file>
	<file>palette.c</file>
	<file>regsvr.c</file>
	<file>stubs.c</file>
	<file>surface.c</file>
	<file>utils.c</file>
	<file>version.rc</file>
	<file>vertexbuffer.c</file>
	<file>viewport.c</file>

	<dependency>wineheaders</dependency>
</module>

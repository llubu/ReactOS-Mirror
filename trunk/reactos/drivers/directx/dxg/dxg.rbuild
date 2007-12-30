<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dxg" type="kernelmodedriver"
installbase="system32/drivers" installname="dxg.sys">
	<importlibrary definition="dxg.def" />
	<include base="dxg">.</include>
	<define name="__USE_W32API" />
	<library>dxgthk</library>
	<library>ntoskrnl</library>
	<file>main.c</file>
	<file>ddhmg.c</file>
	<file>eng.c</file>
	<file>historic.c</file>
	<file>dxg.rc</file>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="drmk" type="kernelmodedriver" installbase="system32/drivers" installname="drmk.sys">
	<include base="drmk">.</include>
	<include base="drmk">..</include>
	<include base="drmk">../include</include>
	<importlibrary definition="drmk.spec" />
	<library>ntoskrnl</library>
	<define name="BUILDING_DRMK" />
	<file>stubs.cpp</file>
	<file>drmk.rc</file>
</module>

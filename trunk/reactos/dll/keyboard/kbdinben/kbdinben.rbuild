<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinben" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinben.dll" allowwarnings="true">
	<importlibrary definition="kbdinben.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdinben.c</file>
	<file>kbdinben.rc</file>
	<file>kbdinben.spec</file>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbga" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbga.dll" allowwarnings="true">
	<importlibrary definition="kbdbga.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbga.c</file>
	<file>kbdbga.rc</file>
	<file>kbdbga.spec</file>
</module>

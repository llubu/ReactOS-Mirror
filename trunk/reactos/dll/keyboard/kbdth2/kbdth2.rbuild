<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth2" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth2.dll" allowwarnings="true">
	<importlibrary definition="kbdth2.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdth2.c</file>
	<file>kbdth2.rc</file>
	<file>kbdth2.spec</file>
</module>

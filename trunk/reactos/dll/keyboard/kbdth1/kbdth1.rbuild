<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth1.dll" allowwarnings="true">
	<importlibrary definition="kbdth1.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdth1.c</file>
	<file>kbdth1.rc</file>
	<file>kbdth1.spec</file>
</module>

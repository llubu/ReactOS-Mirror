<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdarme" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdarme.dll" allowwarnings="true">
	<importlibrary definition="kbdarme.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdarme.c</file>
	<file>kbdarme.rc</file>
	<file>kbdarme.spec</file>
</module>

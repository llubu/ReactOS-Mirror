<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdpo" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdpo.dll" allowwarnings="true">
	<importlibrary definition="kbdpo.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdpo.c</file>
	<file>kbdpo.rc</file>
</module>

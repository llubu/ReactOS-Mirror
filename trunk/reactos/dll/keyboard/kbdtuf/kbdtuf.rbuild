<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdtuf" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdtuf.dll" allowwarnings="true">
	<importlibrary definition="kbdtuf.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdtuf.c</file>
	<file>kbdtuf.rc</file>
</module>

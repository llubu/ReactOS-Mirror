<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdhu" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdhu.dll" allowwarnings="true">
	<importlibrary definition="kbdhu.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdhu.c</file>
	<file>kbdhu.rc</file>
	<file>kbdhu.spec</file>
</module>

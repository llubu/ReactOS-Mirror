<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinasa" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinasa.dll" allowwarnings="true">
	<importlibrary definition="kbdinasa.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdinasa.c</file>
	<file>kbdinasa.rc</file>
</module>

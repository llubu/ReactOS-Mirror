<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbgt" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbgt.dll" allowwarnings="true">
	<importlibrary definition="kbdbgt.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbgt.c</file>
	<file>kbdbgt.rc</file>
	<file>kbdbgt.spec</file>
</module>

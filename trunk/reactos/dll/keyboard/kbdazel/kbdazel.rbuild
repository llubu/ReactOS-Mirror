<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdazel" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdazel.dll" allowwarnings="true">
	<importlibrary definition="kbdazel.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdazel.c</file>
	<file>kbdazel.rc</file>
	<file>kbdazel.spec</file>
</module>

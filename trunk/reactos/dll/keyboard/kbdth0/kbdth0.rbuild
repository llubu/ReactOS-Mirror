<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth0" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth0.dll" allowwarnings="true">
	<importlibrary definition="kbdth0.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdth0.c</file>
	<file>kbdth0.rc</file>
	<file>kbdth0.spec</file>
</module>

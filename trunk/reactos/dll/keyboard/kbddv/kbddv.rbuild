<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbddv" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbddv.dll" allowwarnings="true">
	<importlibrary definition="kbddv.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbddv.c</file>
	<file>kbddv.rc</file>
	<file>kbddv.spec</file>
</module>

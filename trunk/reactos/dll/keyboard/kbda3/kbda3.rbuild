<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda3" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbda3.dll" allowwarnings="true">
	<importlibrary definition="kbda3.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbda3.c</file>
	<file>kbda3.rc</file>
	<file>kbda3.spec</file>
</module>

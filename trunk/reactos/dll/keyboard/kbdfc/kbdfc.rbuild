<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdfc" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdfc.dll" allowwarnings="true">
	<importlibrary definition="kbdfc.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdfc.c</file>
	<file>kbdfc.rc</file>
</module>

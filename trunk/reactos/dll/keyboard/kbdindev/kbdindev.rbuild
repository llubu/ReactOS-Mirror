<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdindev" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdindev.dll" allowwarnings="true">
	<importlibrary definition="kbdindev.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdindev.c</file>
	<file>kbdindev.rc</file>
</module>

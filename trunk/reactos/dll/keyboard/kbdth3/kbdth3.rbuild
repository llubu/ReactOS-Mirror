<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth3" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdth3.dll" allowwarnings="true">
	<importlibrary definition="kbdth3.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdth3.c</file>
	<file>kbdth3.rc</file>
</module>

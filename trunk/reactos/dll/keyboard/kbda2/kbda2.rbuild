<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda2" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbda2.dll" allowwarnings="true">
	<importlibrary definition="kbda2.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbda2.c</file>
	<file>kbda2.rc</file>
</module>

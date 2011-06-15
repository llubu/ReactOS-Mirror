<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<group>
<module name="newinflib" type="staticlibrary">
	<include base="newinflib">.</include>
	<file>infcore.c</file>
	<file>infget.c</file>
	<file>infput.c</file>
	<file>infrosgen.c</file>
	<file>infrosget.c</file>
	<file>infrosput.c</file>
</module>
<module name="newinflibhost" type="hoststaticlibrary" allowwarnings="true">
	<include base="newinflibhost">.</include>
	<define name="__NO_CTYPE_INLINES" />
	<group compilerset="gcc">
		<compilerflag>-Wwrite-strings</compilerflag>
		<compilerflag>-Wpointer-arith</compilerflag>
	</group>
	<define name="INFLIB_HOST" />
	<file>infcore.c</file>
	<file>infget.c</file>
	<file>infput.c</file>
	<file>infhostgen.c</file>
	<file>infhostget.c</file>
	<file>infhostput.c</file>
	<file>infhostrtl.c</file>
</module>
</group>

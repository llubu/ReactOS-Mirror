<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="mkhive" type="buildtool" allowwarnings="true">
	<include base="ReactOS">include</include>
	<include base="ReactOS">include/reactos</include>
	<include base="inflibhost">.</include>
	<include base="cmlibhost">.</include>
	<include base="zlibhost">.</include>
	<include base="rtl">.</include>
	<define name="MKHIVE_HOST" />
	<compilerflag>-fshort-wchar</compilerflag>
	<library>inflibhost</library>
	<library>cmlibhost</library>
	<file>binhive.c</file>
	<file>cmi.c</file>
	<file>mkhive.c</file>
	<file>reginf.c</file>
	<file>registry.c</file>
	<file>rtl.c</file>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe" unicode="yes" allowwarnings="true">
	<include base="wordpad">.</include>
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<compilationunit name="unit.c">
		<file>print.c</file>
		<file>registry.c</file>
		<file>wordpad.c</file>
	</compilationunit>
	<file>rsrc.rc</file>
</module>

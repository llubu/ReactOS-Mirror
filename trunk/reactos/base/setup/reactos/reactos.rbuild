<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="reactos" type="win32gui" unicode="yes">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="reactos">.</include>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>setupapi</library>
	<library>uuid</library>
	<file>reactos.c</file>
	<file>reactos.rc</file>
</module>

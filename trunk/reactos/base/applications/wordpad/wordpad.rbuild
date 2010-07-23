<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe" allowwarnings="true">
	<include base="wordpad">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>comdlg32</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<file>olecallback.c</file>
	<file>print.c</file>
	<file>registry.c</file>
	<file>wordpad.c</file>
	<file>rsrc.rc</file>
</module>

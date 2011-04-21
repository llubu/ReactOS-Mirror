<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mspaint" type="win32gui" installbase="system32" installname="mspaint.exe" unicode="yes" allowwarnings="true">
	<include base="mspaint">.</include>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<file>dialogs.c</file>
	<file>dib.c</file>
	<file>drawing.c</file>
	<file>history.c</file>
	<file>main.c</file>
	<file>mouse.c</file>
	<file>palette.c</file>
	<file>registry.c</file>
	<file>selection.c</file>
	<file>sizebox.c</file>
	<file>toolsettings.c</file>
	<file>winproc.c</file>
	<file>rsrc.rc</file>
</module>

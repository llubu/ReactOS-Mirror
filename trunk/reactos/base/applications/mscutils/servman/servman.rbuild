<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="servman" type="win32gui" installbase="system32" installname="servman.exe" unicode="yes">
	<include base="servman">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>version</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>comdlg32</library>
	<compilationunit name="unit.c">
		<file>about.c</file>
		<file>control.c</file>
		<file>create.c</file>
		<file>delete.c</file>
		<file>dependencies.c</file>
		<file>export.c</file>
		<file>listview.c</file>
		<file>mainwnd.c</file>
		<file>misc.c</file>
		<file>progress.c</file>
		<file>propsheet.c</file>
		<file>propsheet_depends.c</file>
		<file>propsheet_general.c</file>
		<file>query.c</file>
		<file>servman.c</file>
		<file>start.c</file>
		<file>stop.c</file>
	</compilationunit>
	<file>servman.rc</file>
	<pch>precomp.h</pch>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sc" type="win32cui" installbase="system32" installname="sc.exe" unicode="yes">
	<define name="DEFINE_GUID" />
	<library>advapi32</library>
	<file>control.c</file>
	<file>create.c</file>
	<file>delete.c</file>
	<file>print.c</file>
	<file>query.c</file>
	<file>sc.c</file>
	<file>start.c</file>
	<file>usage.c</file>
	<file>sc.rc</file>
	<pch>sc.h</pch>
</module>

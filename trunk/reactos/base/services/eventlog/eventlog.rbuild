<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="eventlog" type="win32cui" installbase="system32" installname="eventlog.exe" unicode="yes">
	<include base="eventlog">.</include>
	<include base="eventlog_server">.</include>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>eventlog_server</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>eventlog.c</file>
	<file>eventsource.c</file>
	<file>logport.c</file>
	<file>eventlog.rc</file>
	<file>rpc.c</file>
	<file>file.c</file>
	<pch>eventlog.h</pch>
</module>

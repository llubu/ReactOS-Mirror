<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="services" type="win32cui" installbase="system32" installname="services.exe" unicode="yes">
	<include base="services">.</include>
	<include base="scm_server">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>scm_server</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>config.c</file>
	<file>database.c</file>
	<file>driver.c</file>
	<file>groupdb.c</file>
	<file>rpcserver.c</file>
	<file>services.c</file>
	<file>services.rc</file>
	<pch>services.h</pch>
</module>

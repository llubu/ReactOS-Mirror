<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="netstat" type="win32cui" installbase="system32" installname="netstat.exe">
	<include base="netstat">.</include>
	<library>user32</library>
	<library>ws2_32</library>
	<library>snmpapi</library>
	<library>iphlpapi</library>
	<file>netstat.c</file>
	<file>netstat.rc</file>
</module>

<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="net" type="win32cui" installbase="system32" installname="net.exe">
	<include base="ping">.</include>
	<define name="__USE_W32_SOCKETS" />
	<library>ws2_32</library>
	<file>main.c</file>
	<file>cmdstart.c</file>
	<file>cmdStop.c</file>
	<file>help.c</file>
	<file>process.c</file>
</module>

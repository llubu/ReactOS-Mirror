<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="lsass" type="win32gui" installbase="system32" installname="lsass.exe">
	<include base="lsass">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>lsasrv</library>
	<file>lsass.c</file>
	<file>lsass.rc</file>
</module>

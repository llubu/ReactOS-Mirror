<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="winlogon" type="win32gui" installbase="system32" installname="winlogon.exe">
	<include base="winlogon">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>userenv</library>
	<library>secur32</library>
	<file>sas.c</file>
	<file>setup.c</file>
	<file>winlogon.c</file>
	<file>wlx.c</file>
	<file>winlogon.rc</file>
	<pch>winlogon.h</pch>
</module>

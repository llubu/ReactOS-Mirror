<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="expand" type="win32cui" installbase="system32" installname="expand.exe" usewrc="false">
	<include base="ReactOS">include/reactos/wine</include>
	<include base="cmd">.</include>
	<define name="__USE_W32API" />
	<define name="ANONYMOUSUNIONS" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>lz32</library>
	<library>user32</library>
	<library>kernel32</library>
	<file>expand.c</file>	
	<file>expand.rc</file> 
</module>

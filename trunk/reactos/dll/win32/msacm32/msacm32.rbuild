<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msacm32" type="win32dll" baseaddress="${BASEADDRESS_MSACM32}" installbase="system32" installname="msacm32.dll" unicode="yes">
	<importlibrary definition="msacm32.spec" />
	<include base="msacm32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>driver.c</file>
	<file>filter.c</file>
	<file>format.c</file>
	<file>internal.c</file>
	<file>msacm32_main.c</file>
	<file>pcmconverter.c</file>
	<file>stream.c</file>
</module>
<directory name="msacm32.drv">
	<xi:include href="msacm32.drv/msacm32.drv.rbuild" />
</directory>
</group>

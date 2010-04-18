<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<installfile installbase="system32">downloader.xml</installfile>
<module name="downloader" type="win32gui" installbase="system32" installname="downloader.exe" unicode="yes">
	<include base="downloader">.</include>
	<include base="expat">.</include>

	<library>advapi32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>msimg32</library>
	<library>shlwapi</library>
	<library>urlmon</library>
	<library>uuid</library>
	<library>expat</library>

	<file>main.c</file>
	<file>xml.c</file>
	<file>download.c</file>
	<file>downloader.rc</file>
</module>
</group>

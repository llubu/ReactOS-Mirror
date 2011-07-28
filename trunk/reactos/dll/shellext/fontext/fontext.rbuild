<module name="fontext" type="win32dll" baseaddress="${BASEADDRESS_FONTEXT}" installbase="system32" installname="fontext.dll" unicode="yes">
	<importlibrary definition="fontext.spec" />
	<include base="fontext">.</include>
	<library>ntdll</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shlwapi</library>
	<library>lz32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<file>fontext.c</file>
	<file>regsvr.c</file>
	<file>fontext.rc</file>
	<pch>fontext.h</pch>
</module>

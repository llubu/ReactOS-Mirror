<module name="midimap" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MIDIMAP}" installbase="system32" installname="midimap.dll" allowwarnings="true" unicode="yes">
	<importlibrary definition="midimap.spec.def" />
	<include base="midimap">.</include>
	<include base="ReactOS">include/wine</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<library>msvcrt</library>
	<file>midimap.c</file>
	<file>midimap.rc</file>
	<file>midimap.spec</file>
</module>

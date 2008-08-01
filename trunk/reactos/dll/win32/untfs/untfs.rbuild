<module name="untfs" type="win32dll" baseaddress="${BASEADDRESS_UNTFS}" installbase="system32" installname="untfs.dll">
	<importlibrary definition="untfs.def" />
	<include base="untfs">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0600</define>
	<library>ntfslib</library>
	<library>ntdll</library>
	<pch>precomp.h</pch>
	<file>untfs.c</file>
	<file>untfs.rc</file>
</module>

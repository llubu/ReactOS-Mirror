<module name="wshirda" type="win32dll" baseaddress="${BASEADDRESS_WSHIRDA}" installbase="system32" installname="wshirda.dll" unicode="yes">
	<importlibrary definition="wshirda.def" />
	<include base="wshirda">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<pch>wshirda.h</pch>
	<file>wshirda.c</file>
	<file>wshirda.rc</file>
</module>

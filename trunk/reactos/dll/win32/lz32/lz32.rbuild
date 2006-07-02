<module name="lz32" type="win32dll" baseaddress="${BASEADDRESS_LZ32}" installbase="system32" installname="lz32.dll">
	<importlibrary definition="lz32.def" />
	<include base="lz32">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<file>lzexpand_main.c</file>
	<file>lz32.rc</file>
</module>

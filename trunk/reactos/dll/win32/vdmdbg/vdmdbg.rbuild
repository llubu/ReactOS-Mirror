<module name="vdmdbg" type="win32dll" baseaddress="${BASEADDRESS_VDMDBG}" installbase="system32" installname="vdmdbg.dll" unicode="yes">
	<importlibrary definition="vdmdbg.spec" />
	<include base="vdmdbg">.</include>
	<library>ntdll</library>
	<file>vdmdbg.c</file>
	<pch>vdmdbg.h</pch>
</module>

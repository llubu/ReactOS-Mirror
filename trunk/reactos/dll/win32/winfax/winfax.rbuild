<module name="winfax" type="win32dll" baseaddress="${BASEADDRESS_WINFAX}" installbase="system32" installname="winfax.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="winfax.spec" />
	<include base="winfax">.</include>
	<library>ntdll</library>
	<file>winfax.c</file>
	<file>winfax.rc</file>
</module>

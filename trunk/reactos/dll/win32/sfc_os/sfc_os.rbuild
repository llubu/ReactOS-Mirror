<module name="sfc_os" type="win32dll" baseaddress="${BASEADDRESS_SFC_OS}" installbase="system32" installname="sfc_os.dll" allowwarnings="yes">
	<importlibrary definition="sfc_os.spec" />
	<include base="sfc_os">.</include>
	<file>sfc_os.c</file>
	<pch>precomp.h</pch>
</module>

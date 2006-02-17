<module name="oledlg" type="win32dll" baseaddress="${BASEADDRESS_OLEDLG}" installbase="system32" installname="oledlg.dll" allowwarnings="true">
	<importlibrary definition="oledlg.spec.def" />
	<include base="oledlg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>comdlg32</library>
	<library>ole32</library>
	<file>oledlg_main.c</file>
	<file>insobjdlg.c</file>
	<file>rsrc.rc</file>
	<file>oledlg.spec</file>
</module>

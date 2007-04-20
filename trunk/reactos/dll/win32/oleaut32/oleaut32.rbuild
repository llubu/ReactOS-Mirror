<module name="oleaut32" type="win32dll" baseaddress="${BASEADDRESS_OLEAUT32}" installbase="system32" installname="oleaut32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="oleaut32.spec.def" />
	<include base="oleaut32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_STDDEF_H" />
	<define name="_OLEAUT32_" />
	<define name="COM_NO_WINDOWS_H" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>rpcrt4</library>
	<library>ole32</library>
	<library>comctl32</library>
	<library>urlmon</library>
	<file>connpt.c</file>
	<file>dispatch.c</file>
	<file>hash.c</file>
	<file>oaidl_p.c</file>
	<file>oleaut.c</file>
	<file>olefont.c</file>
	<file>olepicture.c</file>
	<file>recinfo.c</file>
	<file>regsvr.c</file>
	<file>safearray.c</file>
	<file>stubs.c</file>
	<file>tmarshal.c</file>
	<file>typelib.c</file>
	<file>typelib2.c</file>
	<file>usrmarshal.c</file>
	<file>varformat.c</file>
	<file>variant.c</file>
	<file>ungif.c</file>
	<file>vartype.c</file>
	<file>oleaut32.spec</file>
</module>

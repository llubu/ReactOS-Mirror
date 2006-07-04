<module name="oleacc" type="win32dll" baseaddress="${BASEADDRESS_OLEACC}" installbase="system32" installname="oleacc.dll" allowwarnings="true">
	<importlibrary definition="oleacc.spec.def" />
	<include base="oleacc">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>main.c</file>
	<file>oleacc.spec</file>
</module>

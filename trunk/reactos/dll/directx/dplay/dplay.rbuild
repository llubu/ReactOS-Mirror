<module name="dplay" type="win32dll" baseaddress="${BASEADDRESS_DPLAY}" installbase="system32" installname="dplay.dll">  
	<importlibrary definition="dplay.spec.def" />
	<include base="dinput8">.</include>
	<include base="ReactOS">include/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>dxguid</library>
	<library>dinput</library>
      <file>version.rc</file>
	<file>dplay_main.c</file>
	<file>dplay.spec</file>
</module>

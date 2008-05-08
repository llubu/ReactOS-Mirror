<module name="msacm32" type="win32dll" baseaddress="${BASEADDRESS_MSACM32}" installbase="system32" installname="msacm32.dll" unicode="yes">
	<importlibrary definition="msacm32.spec.def" />
	<include base="msacm32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>driver.c</file>
	<file>filter.c</file>
	<file>format.c</file>
	<file>internal.c</file>
	<file>msacm32_main.c</file>
	<file>pcmconverter.c</file>
	<file>stream.c</file>
	<file>msacm32.spec</file>
</module>

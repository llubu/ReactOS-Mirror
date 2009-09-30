<module name="snmpapi" type="win32dll" baseaddress="${BASEADDRESS_SNMPAPI}" installbase="system32" installname="snmpapi.dll" unicode="yes">
	<importlibrary definition="snmpapi.spec" />
	<include base="snmpapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>wine</library>
	<file>main.c</file>
	<file>snmpapi.c</file>
	<file>snmpapi.rc</file>
</module>

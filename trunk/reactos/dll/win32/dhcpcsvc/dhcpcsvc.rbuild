<module name="dhcpcsvc" type="win32dll" baseaddress="${BASEADDRESS_DHCPCSVC}" installbase="system32" installname="dhcpcsvc.dll">
	<importlibrary definition="dhcpcsvc.spec" />
	<include base="dhcpcsvc">include</include>
	<library>ntdll</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<file>dhcpcsvc.c</file>
	<file>dhcpcsvc.rc</file>
</module>

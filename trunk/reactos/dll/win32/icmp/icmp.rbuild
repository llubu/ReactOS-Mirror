<module name="icmp" type="win32dll" baseaddress="${BASEADDRESS_ICMP}" installbase="system32" installname="icmp.dll">
	<importlibrary definition="icmp.spec" />
	<include base="icmp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>ws2_32</library>
	<library>wine</library>
	<file>icmp_main.c</file>
	<file>icmp.rc</file>
</module>

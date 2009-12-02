<module name="winsta" type="win32dll" baseaddress="${BASEADDRESS_WINSTA}" installbase="system32" installname="winsta.dll">
	<importlibrary definition="winsta.spec" />
	<include base="winsta">.</include>
	<library>wine</library>
	<library>ntdll</library>
	<pch>winsta.h</pch>
	<file>logon.c</file>
	<file>main.c</file>
	<file>misc.c</file>
	<file>query.c</file>
	<file>security.c</file>
	<file>server.c</file>
	<file>ws.c</file>
	<file>winsta.rc</file>
</module>

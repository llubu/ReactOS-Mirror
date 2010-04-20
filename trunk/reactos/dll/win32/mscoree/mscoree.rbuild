<module name="mscoree" type="win32dll" baseaddress="${BASEADDRESS_MSCOREE}" installbase="system32" installname="mscoree.dll">
	<importlibrary definition="mscoree.spec" />
	<include base="mscoree">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>uuid</library>
	<file>corruntimehost.c</file>
	<file>mscoree_main.c</file>
</module>

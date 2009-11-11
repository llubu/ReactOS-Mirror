<module name="crypt32" type="win32dll" baseaddress="${BASEADDRESS_CRYPT32}" installbase="system32" installname="crypt32.dll" allowwarnings="true" crt="msvcrt">
	<importlibrary definition="crypt32.spec" />
	<include base="crypt32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>

	<!-- FIXME: workarounds until we have a proper oldnames library -->
	<define name="fdopen">_fdopen</define>
	<define name="open">_open</define>
	<define name="close">_close</define>

	<library>wine</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>imagehlp</library>
	<library>pseh</library>
	<file>base64.c</file>
	<file>cert.c</file>
	<file>chain.c</file>
	<file>collectionstore.c</file>
	<file>context.c</file>
	<file>crl.c</file>
	<file>decode.c</file>
	<file>encode.c</file>
	<file>filestore.c</file>
	<file>main.c</file>
	<file>msg.c</file>
	<file>object.c</file>
	<file>oid.c</file>
	<file>proplist.c</file>
	<file>protectdata.c</file>
	<file>provstore.c</file>
	<file>regstore.c</file>
	<file>rootstore.c</file>
	<file>serialize.c</file>
	<file>sip.c</file>
	<file>store.c</file>
	<file>str.c</file>
	<file>ctl.c</file>
	<file>message.c</file>
	<file>crypt32.rc</file>
	<file>version.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>

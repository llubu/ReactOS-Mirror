<module name="msvcrt" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT}" installbase="system32" installname="msvcrt.dll" iscrt="yes">
	<importlibrary definition="msvcrt.spec" />
	<include base="msvcrt">.</include>
	<include base="crt">include</include>
	<define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MT" />
	<define name="_CTYPE_DISABLE_MACROS" />
	<define name="_NO_INLINING" />
	<define name="CRTDLL" />
	<linkerflag linkerset="ld">-enable-stdcall-fixup</linkerflag>

	<!--	__MINGW_IMPORT needs to be defined differently because it's defined
		as dllimport by default, which is invalid from GCC 4.1.0 on!	-->
	<define name="__MINGW_IMPORT">"extern __attribute__ ((dllexport))"</define>

	<library>crt</library>
	<library>wine</library>
	<library>ntdll</library>
	<library>pseh</library>
	<pch>precomp.h</pch>
	<file>dllmain.c</file>
	<file>msvcrt.rc</file>
</module>

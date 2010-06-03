<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="quartz" type="win32dll" baseaddress="${BASEADDRESS_QUARTZ}" installbase="system32" installname="quartz.dll" allowwarnings="true" crt="MSVCRT">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="quartz.spec" />
	<include base="quartz">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<define name="ENTRY_PREFIX">QUARTZ_</define>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="PROXY_DELEGATION"/>
	<library>wine</library>
	<library>uuid</library>
	<library>advapi32</library>
	<library>dsound</library>
	<library>strmiids</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>shlwapi</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>msvfw32</library>
	<library>msacm32</library>
	<library>ntdll</library>
	<library>quartz_proxy</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>avidec.c</file>
	<file>acmwrapper.c</file>
	<file>waveparser.c</file>
	<file>videorenderer.c</file>
	<file>transform.c</file>
	<file>systemclock.c</file>
	<file>regsvr.c</file>
	<file>pin.c</file>
	<file>parser.c</file>
	<file>nullrenderer.c</file>
	<file>mpegsplit.c</file>
	<file>memallocator.c</file>
	<file>main.c</file>
	<file>filtermapper.c</file>
	<file>filtergraph.c</file>
	<file>filesource.c</file>
	<file>enumregfilters.c</file>
	<file>enumpins.c</file>
	<file>enummoniker.c</file>
	<file>enumfilters.c</file>
	<file>dsoundrender.c</file>
	<file>enummedia.c</file>
	<file>control.c</file>
	<file>avisplit.c</file>
	<file>version.rc</file>
</module>
<module name="quartz_proxy" type="rpcproxy" allowwarnings="true">
	<define name="__WINESRC__" />
	<define name="ENTRY_PREFIX">QUARTZ_</define>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="PROXY_DELEGATION"/>
	<file>quartz_strmif.idl</file>
</module>
</group>
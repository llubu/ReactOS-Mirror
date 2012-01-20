<module name="netshell" type="win32dll" baseaddress="${BASEADDRESS_NETSHELL}" installbase="system32" installname="netshell.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="netshell.spec" />
	<include base="netshell">.</include>
	<define name="_NETSHELL_" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>shlwapi</library>
	<library>shell32</library>
	<library>version</library>
	<library>iphlpapi</library>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>uuid</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>ws2_32</library>
	<library>comctl32</library>
	<pch>precomp.h</pch>
	<file>netshell.cpp</file>
	<file>shfldr_netconnect.cpp</file>
	<file>enumlist.cpp</file>
	<file>netshell.rc</file>
	<file>classfactory.cpp</file>
	<file>connectmanager.cpp</file>
	<file>lanconnectui.cpp</file>
	<file>lanstatusui.cpp</file>
</module>

<module name="rpcrt4" type="win32dll" baseaddress="${BASEADDRESS_RPCRT4}" installbase="system32" installname="rpcrt4.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rpcrt4.spec.def" />
	<include base="rpcrt4">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_STDDEF_H" />
	<define name="_RPCRT4_" />
	<define name="COM_NO_WINDOWS_H" />
	<define name="MSWMSG" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>secur32</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<file>cproxy.c</file>
	<file>cpsf.c</file>
	<file>cstub.c</file>
	<file>ndr_clientserver.c</file>
	<file>ndr_fullpointer.c</file>
	<file>ndr_marshall.c</file>
	<file>ndr_ole.c</file>
	<file>ndr_stubless.c</file>
	<file>rpc_binding.c</file>
	<file>rpc_epmap.c</file>
	<file>rpc_message.c</file>
	<file>rpc_server.c</file>
	<file>rpc_transport.c</file>
	<file>rpcrt4_main.c</file>
	<file>rpcss_np_client.c</file>
	<file>unix_func.c</file>
	<file>rpcrt4.rc</file>
	<file>rpcrt4.spec</file>
</module>

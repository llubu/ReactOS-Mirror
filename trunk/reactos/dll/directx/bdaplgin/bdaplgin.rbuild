<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="bdaplgin" type="win32dll" baseaddress="${BASEADDRESS_BDAPLGIN}" installbase="system32" installname="bdaplgin.ax">
	<importlibrary definition="bdaplgin.spec" />
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<include base="bdaplgin">.</include>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>msvcrt</library>
	<library>ksproxy</library>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<file>bdaplgin.cpp</file>
	<file>bdaplgin.rc</file>
	<file>classfactory.cpp</file>
	<file>controlnode.cpp</file>
	<file>devicecontrol.cpp</file>
	<file>digitaldemo.cpp</file>
	<file>frequencyfilter.cpp</file>
	<file>lnbinfo.cpp</file>
	<file>pincontrol.cpp</file>
	<file>signalstatistics.cpp</file>
</module>
</group>

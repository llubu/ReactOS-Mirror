<module name="cmd_test" type="test">
	<include base="rtshared">.</include>
	<include base="ReactOS">include/wine</include>
	<include base="cmd">.</include>
	<define name="__USE_W32API" />
	<define name="ANONYMOUSUNIONS" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>rtshared</library>
	<library>regtests</library>
	<library>cmd_base</library>
	<library>pseh</library>
	<library>ntdll</library>
	<file>setup.c</file>
	<xi:include href="stubs.rbuild" />
</module>

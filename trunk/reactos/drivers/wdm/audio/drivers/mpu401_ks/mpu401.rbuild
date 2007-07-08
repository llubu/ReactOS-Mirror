<module name="mpu401" type="kernelmodedriver" installbase="system32/drivers" installname="mpu401.sys" allowwarnings="true" entrypoint="DriverEntry">
    <linkerflag>-Wl,--entry,_DriverEntry@8</linkerflag>
	<include base="mpu401">.</include>
	<include base="mpu401">..</include>
    <library>ntoskrnl</library>
    <library>portcls</library>
    <define name="DBG" />
	<define name="__USE_W32API" />
	<!--file>mpu401.rc</file-->
	<file>adapter.cpp</file>
</module>

<module name="disk" type="kernelmodedriver" installbase="system32/drivers" installname="disk.sys" allowwarnings="true">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<linkerflag>-lgcc</linkerflag>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<include base="disk">..</include>
	<file>disk.c</file>
	<file>disk.rc</file>
</module>

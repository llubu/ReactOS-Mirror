<module name="cdrom" type="kernelmodedriver" installbase="system32/drivers" installname="cdrom.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<include base="cdrom">..</include>
	<file>cdrom.c</file>
	<file>cdrom.rc</file>
</module>

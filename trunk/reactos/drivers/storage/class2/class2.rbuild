<module name="class2" type="exportdriver" installbase="system32/drivers" installname="class2.sys" allowwarnings="true">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<importlibrary definition="class2.def" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>scsiport</library>
	<include base="class2">..</include>
	<file>class2.c</file>
	<file>class2.rc</file>
</module>

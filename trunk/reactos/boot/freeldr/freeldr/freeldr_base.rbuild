<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_base" type="objectlibrary">
	<include base="freeldr_base">include</include>
	<include base="freeldr_base">cache</include>
	<include base="cmlib">.</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<define name="_NTSYSTEM_" />
	<directory name="arcemul">
		<file>mm.c</file>
		<file>time.c</file>
	</directory>
	<directory name="cache">
		<file>blocklist.c</file>
		<file>cache.c</file>
	</directory>
	<directory name="comm">
		<file>rs232.c</file>
	</directory>
	<directory name="disk">
		<file>disk.c</file>
		<file>partition.c</file>
		<file>ramdisk.c</file>
	</directory>
	<directory name="fs">
		<file>ext2.c</file>
		<file>fat.c</file>
		<file>fs.c</file>
		<file>iso.c</file>
		<file>ntfs.c</file>
	</directory>
	<directory name="inifile">
		<file>ini_init.c</file>
		<file>inifile.c</file>
		<file>parse.c</file>
	</directory>
	<directory name="mm">
		<file>meminit.c</file>
		<file>mm.c</file>
	</directory>
	<directory name="reactos">
		<file>registry.c</file>
		<file>arcname.c</file>
		<file>archwsup.c</file>
		<file>binhive.c</file>
		<file>reactos.c</file>
        <file>imageldr.c</file>
	</directory>
	<directory name="rtl">
		<file>bget.c</file>
		<file>libsupp.c</file>
	</directory>
	<directory name="ui">
	    <file>directui.c</file>
		<file>gui.c</file>
		<file>minitui.c</file>
		<file>noui.c</file>
		<file>tui.c</file>
		<file>tuimenu.c</file>
		<file>ui.c</file>
	</directory>
	<directory name="video">
		<file>fade.c</file>
		<file>palette.c</file>
		<file>video.c</file>
	</directory>
	<directory name="windows">
		<file>conversion.c</file>
		<file>peloader.c</file>
		<file>winldr.c</file>
		<file>wlmemory.c</file>
		<file>wlregistry.c</file>
	</directory>
	<file>freeldr.c</file>
	<file>debug.c</file>
	<file>version.c</file>
	<file>cmdline.c</file>
	<file>machine.c</file>
	<directory name="include">
		<pch>freeldr.h</pch>
	</directory>
	<file>options.c</file>
	<file>linuxboot.c</file>
	<file>oslist.c</file>
</module>

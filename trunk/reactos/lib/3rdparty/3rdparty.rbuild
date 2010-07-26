<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<group compilerset="gcc">
		<compilerflag compiler="cc">-Wno-unused-value</compilerflag>
	</group>
	<directory name="adns">
		<xi:include href="adns/adns.rbuild" />
	</directory>
	<directory name="bzip2">
		<xi:include href="bzip2/bzip2.rbuild" />
	</directory>
	<directory name="cardlib">
		<xi:include href="cardlib/cardlib.rbuild" />
	</directory>
	<directory name="expat">
		<xi:include href="expat/expat.rbuild" />
	</directory>
	<directory name="freetype">
		<xi:include href="freetype/freetype.rbuild" />
	</directory>
	<directory name="fullfat">
		<xi:include href="fullfat/fullfat.rbuild" />
	</directory>
	<directory name="icu4ros">
		<xi:include href="icu4ros/icu4ros.rbuild" />
	</directory>
	<directory name="libmpg123">
		<xi:include href="libmpg123/libmpg123.rbuild" />
	</directory>
	<directory name="libsamplerate">
		<xi:include href="libsamplerate/libsamplerate.rbuild" />
	</directory>
	<directory name="libwine">
		<xi:include href="libwine/libwine.rbuild" />
	</directory>
	<directory name="libxml2">
		<xi:include href="libxml2/libxml2.rbuild" />
	</directory>
	<directory name="mingw">
		<xi:include href="mingw/mingw.rbuild" />
	</directory>
	<directory name="zlib">
		<xi:include href="zlib/zlib.rbuild" />
	</directory>
</group>

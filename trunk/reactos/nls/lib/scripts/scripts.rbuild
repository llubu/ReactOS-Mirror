<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="scripts" type="staticlibrary">
	<library>icu4ros</library>
	<compilerflag>-fno-exceptions</compilerflag>
	<compilerflag>-fno-rtti</compilerflag>
	<include base="icu4ros">icu/source/common</include>
	<file>scripts.cpp</file>
</module>

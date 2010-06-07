<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_pcat" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHALDLL_" />
		<define name="_NTHAL_" />
		<directory name="generic">
    		<directory name="legacy">
        		<directory name="bus">
    				<file>bushndlr.c</file>
    				<file>cmosbus.c</file>
    				<file>isabus.c</file>
    				<file>pcibus.c</file>
    				<file>pcidata.c</file>
    				<file>sysbus.c</file>
    			</directory>
    		    <file>bussupp.c</file>
    	        <file>halpcat.c</file>
    	    </directory>
		</directory>
	</module>
</group>

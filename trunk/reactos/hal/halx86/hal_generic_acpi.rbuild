<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_acpi" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHALDLL_" />
		<define name="_NTHAL_" />
		<directory name="generic">
    	    <directory name="acpi">
    	        <file>halacpi.c</file>
    	        <file>halpnpdd.c</file>
    	        <file>busemul.c</file>
    	    </directory>
    	    <directory name="legacy">
        		<directory name="bus">
    				<file>pcibus.c</file>
    			</directory>
    		</directory>
		</directory>
	</module>
</group>

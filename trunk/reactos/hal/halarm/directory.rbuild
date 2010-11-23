<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">

	<xi:include href="hal_generic.rbuild" />

	<if property="SARCH" value="versatile">
		<xi:include href="versa/halup.rbuild" />
	</if>

	<if property="SARCH" value="omap3-beagle">
		<xi:include href="omap3/halup.rbuild" />
	</if>
	
	<if property="SARCH" value="omap3-zoom2">
		<xi:include href="omap3/halup.rbuild" />
	</if>
</group>

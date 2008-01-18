<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="pnp_server" type="rpcserver" allowwarnings="true">
	<file>pnp.idl</file>
</module>
<module name="pnp_client" type="rpcclient">
	<file>pnp.idl</file>
</module>
<module name="scm_server" type="rpcserver">
	<file>svcctl.idl</file>
</module>
<module name="scm_client" type="rpcclient">
	<file>svcctl.idl</file>
</module>
<module name="eventlog_server" type="rpcserver" allowwarnings="true">
	<file>eventlogrpc.idl</file>
</module>
<module name="eventlog_client" type="rpcclient" >
	<file>eventlogrpc.idl</file>
</module>
<module name="lsa_server" type="rpcserver">
	<file>lsa.idl</file>
</module>
<module name="lsa_client" type="rpcclient">
	<file>lsa.idl</file>
</module>
</group>

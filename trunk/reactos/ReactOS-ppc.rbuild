<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.ppc" xmlns:xi="http://www.w3.org/2001/XInclude">
  <xi:include href="config-ppc.rbuild">
    <xi:fallback>
      <xi:include href="config.template.rbuild" />
    </xi:fallback>
  </xi:include>

  <xi:include href="baseaddress.rbuild" />

  <property name="BOOTPROG_PREPARE" value="ppc-le2be" />
  <property name="BOOTPROG_FLATFORMAT" value="-O elf32-powerpc -B powerpc:common" />
  <property name="BOOTPROG_LINKFORMAT" value="-melf32ppc --no-omagic -Ttext 0xe00000 -Tdata 0xe10000" />
  <property name="BOOTPROG_COPYFORMAT" value="--only-section=.text --only-section=.data --only-section=.bss -O aixcoff-rs6000" />

  <define name="_M_PPC" />
  <define name="_PPC_" />
  <define name="__PowerPC__" />
  <define name="_REACTOS_" />
  <define name="__MINGW_IMPORT" empty="true" />
  <define name="__restrict__" empty="true" />
  <compilerflag>-v</compilerflag>
  <if property="MP" value="1">
    <define name="CONFIG_SMP" value="1" />
  </if>
  <if property="DBG" value="1">
    <define name="DBG" value="1" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>
  <if property="DBG" value="0">
    <compilerflag>-Os</compilerflag>
    <compilerflag>-Wno-strict-aliasing</compilerflag>
  </if>
  <if property="KDBG" value="1">
    <define name="KDBG" value="1" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>
  <compilerflag>-Wpointer-arith</compilerflag>

  <include>.</include>
  <include>include</include>
  <include>include/reactos</include>
  <include>include/libs</include>
  <include>include/drivers</include>
  <include>include/subsys</include>
  <include>include/ndk</include>
  <include>w32api/include</include>
  <include>w32api/include/crt</include>
  <include>w32api/include/ddk</include>

  <directory name="apps">
    <xi:include href="apps/directory.rbuild" />
  </directory>
  <directory name="boot">
    <xi:include href="boot/boot.rbuild" />
  </directory>
  <directory name="drivers">
    <xi:include href="drivers/directory.rbuild" />
  </directory>
  <directory name="hal">
    <xi:include href="hal/directory.rbuild" />
  </directory>
  <directory name="include">
    <xi:include href="include/directory.rbuild" />
  </directory>
  <directory name="lib">
    <xi:include href="lib/directory.rbuild" />
  </directory>
  <directory name="media">
    <xi:include href="media/directory.rbuild" />
  </directory>
  <directory name="modules">
    <xi:include href="modules/directory.rbuild" />
  </directory>
  <directory name="ntoskrnl">
    <xi:include href="ntoskrnl/ntoskrnl.rbuild" />
  </directory>
  <directory name="regtests">
    <xi:include href="regtests/directory.rbuild" />
  </directory>
  <directory name="services">
    <xi:include href="services/directory.rbuild" />
  </directory>
  <directory name="subsys">
    <xi:include href="subsys/directory.rbuild" />
  </directory>
</project>

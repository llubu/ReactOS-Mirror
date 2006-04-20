<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">

<!--
  This file is a template used as a starting point for compile-time
  configuration of ReactOS. Make a copy of this file and name it config.rbuild.
  Then change the options in config.rbuild. If you don't have a config.rbuild file,
  then the defaults in this file, config.template.rbuild, will be used instead.

  Boolean options can obtain the values 0 (disabled) or 1 (enabled). String
  options can obtain any value specified in the comment before it.
-->


<!--
  Architecture to build for. Specify one of:
    i386
-->
<property name="ARCH" value="i386" />

<!--
  Sub-architecture to build for. Specify one of:
    xbox
-->
<property name="SARCH" value="" />


<!--
  Which CPU ReactOS should be optimized for. Specify one of:
    i486, i586, pentium, pentium2, pentium3, pentium4, athlon-xp, athlon-mp,
    k6-2

  See GCC manual for more CPU names and which CPUs GCC can optimize for.
-->
<property name="OARCH" value="i486" />


<!--
  Whether to compile for an uniprocessor or multiprocessor machine.
-->
<property name="MP" value="0" />


<!--
  Whether to compile in the integrated kernel debugger.
-->
<property name="KDBG" value="0" />


<!--
  Whether to compile for debugging. No compiler optimizations will be
  performed.
-->
<property name="DBG" value="1" />


<!--
  Whether to compile for debugging with GDB. If you don't use GDB, don't
  enable this.
-->
<property name="GDB" value="0" />

</rbuild>

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


<!--
  Whether to compile apps/libs with features covered software patents or not.
  If you live in a country where software patents are valid/apply, don't
  enable this (except they/you purchased a license from the patent owner).
  This settings is disabled (0) by default.
-->
<property name="NSWPAT" value="0" />

<!--
  Whether to compile with NT-compatible LPC Semantics. At the moment, this will
  cause all LPC-related functionality to fail and should only be used if you're
  working on the \ntlpc directory. Leave this disabled unless you really know
  what you're doing.
-->
<property name="NTLPC" value="0" />

</rbuild>

# $Id: Makefile,v 1.180 2003/11/24 14:25:28 weiden Exp $
#
# Global makefile
#

PATH_TO_TOP = .

#
# Define to build ReactOS external targets
#
ifeq ($(ROS_BUILD_EXT),)
ROS_BUILD_EXT = no
else
ROS_BUILD_EXT = yes
endif

include $(PATH_TO_TOP)/rules.mak

# Required to run the system
COMPONENTS = iface_native iface_additional hallib ntoskrnl

# Hardware Abstraction Layers
# halx86
HALS = halx86

# Bus drivers
# acpi isapnp pci
BUS = acpi isapnp pci

# Filesystem libraries
# vfatlib
LIB_FSLIB = vfatlib

# Static libraries
LIB_STATIC = string rosrtl epsapi zlib

# Keyboard layout libraries
DLLS_KBD = kbdus kbdgr kbdfr

# User mode libraries
# advapi32 cards crtdll fmifs gdi32 imagehlp kernel32 libpcap packet msafd msvcrt ntdll
# epsapi psapi richedit rpcrt4 secur32 user32 version ws2help ws2_32 wsock32 wshirda mswsock
# imagehlp
DLLS = advapi32 cards crtdll fmifs freetype gdi32 kernel32 packet lzexpand msafd \
       msvcrt ntdll psapi richedit rpcrt4 secur32 syssetup twain user32 version winedbgc \
       winspool ws2help ws2_32 wsock32 wshirda iphlpapi msgina mswsock msimg32 d3d8thk \
       $(DLLS_KBD)

SUBSYS = smss win32k csrss ntvdm

#
# Select the server(s) you want to build
#
#SERVERS = posix linux os2
SERVERS = win32

# Boot loaders
# dos
LOADERS = dos

# Driver support libraries
#bzip2 zlib
DRIVERS_LIB = bzip2

# Kernel mode device drivers
# Obsolete: ide
# beep blue floppy null parallel ramdrv serenum serial vga videoprt
DEVICE_DRIVERS = beep blue debugout floppy null serial vga videoprt bootvid

# Kernel mode input drivers
INPUT_DRIVERS = keyboard mouclass psaux sermouse

# Kernel mode file system drivers
# cdfs ext2 fs_rec ms np vfat
FS_DRIVERS = cdfs fs_rec ms np vfat mup ntfs

# Kernel mode networking drivers
# afd ndis npf tcpip tdi wshtcpip
NET_DRIVERS = afd ndis npf tcpip tdi wshtcpip

# Kernel mode networking device drivers
# ne2000 pcnet
NET_DEVICE_DRIVERS = ne2000 pcnet

# Kernel mode storage drivers
# atapi cdrom class2 disk scsiport
STORAGE_DRIVERS = atapi cdrom class2 disk scsiport diskdump

# System applications
# autochk cmd format gstart services setup usetup welcome winlogon
SYS_APPS = autochk cmd format gstart services setup usetup welcome winlogon

# System services
# rpcss eventlog
SYS_SVC = rpcss eventlog

APPS = tests testsets utils


# External (sub)systems for ReactOS
# rosapps wine posix os2 (requires c++) java (non-existant)
EXTERNALS = rosapps wine posix os2

ifeq ($(ROS_BUILD_EXT),yes)
EXT_MODULES = $(EXTERNALS)
else
EXT_MODULES =
endif

KERNEL_DRIVERS = $(DRIVERS_LIB) $(DEVICE_DRIVERS) $(INPUT_DRIVERS) $(FS_DRIVERS) \
	$(NET_DRIVERS) $(NET_DEVICE_DRIVERS) $(STORAGE_DRIVERS)

# Regression tests
REGTESTS = regtests

all: tools dk implib $(LIB_STATIC) $(COMPONENTS) $(HALS) $(BUS) $(LIB_FSLIB) $(DLLS) $(SUBSYS) \
     $(LOADERS) $(KERNEL_DRIVERS) $(SYS_APPS) $(SYS_SVC) \
     $(APPS) $(EXT_MODULES) $(REGTESTS)

#config: $(TOOLS:%=%_config)

depends: $(LIB_STATIC:%=%_depends) $(LIB_FSLIB:%=%_depends) $(DLLS:%=%_depends) $(SUBSYS:%=%_depends) $(SYS_SVC:%=%_depends) \
         $(EXT_MODULES:%=%_depends) $(POSIX_LIBS:%=%_depends)

implib: $(COMPONENTS:%=%_implib) $(HALS:%=%_implib) $(BUS:%=%_implib) \
        $(LIB_STATIC:%=%_implib) $(LIB_FSLIB:%=%_implib) $(DLLS:%=%_implib) $(LOADERS:%=%_implib) \
        $(KERNEL_DRIVERS:%=%_implib) $(SUBSYS:%=%_implib) \
        $(SYS_APPS:%=%_implib) $(SYS_SVC:%=%_implib) \
        $(APPS:%=%_implib) $(EXT_MODULES:%=%_implib)

clean: tools dk_clean $(HALS:%=%_clean) \
       $(COMPONENTS:%=%_clean) $(BUS:%=%_clean) $(LIB_STATIC:%=%_clean) $(LIB_FSLIB:%=%_clean) $(DLLS:%=%_clean) \
       $(LOADERS:%=%_clean) $(KERNEL_DRIVERS:%=%_clean) $(SUBSYS:%=%_clean) \
       $(SYS_APPS:%=%_clean) $(SYS_SVC:%=%_clean) \
       $(NET_APPS:%=%_clean) \
       $(APPS:%=%_clean) $(EXT_MODULES:%=%_clean) $(REGTESTS:%=%_clean) \
       clean_after tools_clean

clean_after:
	$(RM) $(PATH_TO_TOP)/include/roscfg.h

install: tools install_dirs install_before \
         $(COMPONENTS:%=%_install) $(HALS:%=%_install) $(BUS:%=%_install) \
         $(LIB_STATIC:%=%_install) $(LIB_FSLIB:%=%_install) $(DLLS:%=%_install) $(LOADERS:%=%_install) \
         $(KERNEL_DRIVERS:%=%_install) $(SUBSYS:%=%_install) \
         $(SYS_APPS:%=%_install) $(SYS_SVC:%=%_install) \
         $(APPS:%=%_install) $(EXT_MODULES:%=%_install) $(REGTESTS:%=%_install)

dist: $(TOOLS_PATH)/rcopy$(EXE_POSTFIX) dist_clean dist_dirs \
      $(HALS:%=%_dist) $(COMPONENTS:%=%_dist) $(BUS:%=%_dist) $(LIB_STATIC:%=%_dist) $(LIB_FSLIB:%=%_dist) \
	  $(DLLS:%=%_dist) $(LOADERS:%=%_dist) $(KERNEL_DRIVERS:%=%_dist) $(SUBSYS:%=%_dist) \
      $(SYS_APPS:%=%_dist) $(SYS_SVC:%=%_dist) \
      $(NET_APPS:%=%_dist) \
      $(APPS:%=%_dist) $(EXT_MODULES:%=%_dist)

FREELDR_DIR = ../freeldr

freeldr:
	$(MAKE) -C $(FREELDR_DIR)

bootcd_directory_layout: freeldr
	$(RMKDIR) $(BOOTCD_DIR)
	$(RMKDIR) $(BOOTCD_DIR)/bootdisk
	$(RMKDIR) $(BOOTCD_DIR)/loader
	$(RMKDIR) $(BOOTCD_DIR)/reactos
	$(RMKDIR) $(BOOTCD_DIR)/reactos/system32
	$(CP) ${FREELDR_DIR}/bootsect/isoboot.bin ${BOOTCD_DIR}/../isoboot.bin
	$(CP) ${FREELDR_DIR}/bootsect/dosmbr.bin ${BOOTCD_DIR}/loader/dosmbr.bin
	$(CP) ${FREELDR_DIR}/bootsect/ext2.bin ${BOOTCD_DIR}/loader/ext2.bin
	$(CP) ${FREELDR_DIR}/bootsect/fat.bin ${BOOTCD_DIR}/loader/fat.bin
	$(CP) ${FREELDR_DIR}/bootsect/fat32.bin ${BOOTCD_DIR}/loader/fat32.bin
	$(CP) ${FREELDR_DIR}/bootsect/isoboot.bin ${BOOTCD_DIR}/loader/isoboot.bin
	$(CP) ${FREELDR_DIR}/freeldr/obj/i386/freeldr.sys ${BOOTCD_DIR}/loader/freeldr.sys
	$(CP) ${FREELDR_DIR}/freeldr/obj/i386/setupldr.sys ${BOOTCD_DIR}/loader/setupldr.sys

bootcd_bootstrap_files: $(COMPONENTS:%=%_bootcd) $(HALS:%=%_bootcd) $(BUS:%=%_bootcd) \
	$(LIB_STATIC:%=%_bootcd) $(LIB_FSLIB:%=%_bootcd) $(DLLS:%=%_bootcd) $(KERNEL_DRIVERS:%=%_bootcd) \
	$(SUBSYS:%=%_bootcd) $(SYS_APPS:%=%_bootcd)

bootcd_install_before:
	$(RLINE) bootdata/autorun.inf $(BOOTCD_DIR)/autorun.inf
	$(RLINE) bootdata/readme.txt $(BOOTCD_DIR)/readme.txt
	$(RLINE) bootdata/hivecls.inf $(BOOTCD_DIR)/reactos/hivecls.inf
	$(RLINE) bootdata/hivedef.inf $(BOOTCD_DIR)/reactos/hivedef.inf
	$(RLINE) bootdata/hivesft.inf $(BOOTCD_DIR)/reactos/hivesft.inf
	$(RLINE) bootdata/hivesys.inf $(BOOTCD_DIR)/reactos/hivesys.inf
	$(RLINE) bootdata/txtsetup.sif $(BOOTCD_DIR)/reactos/txtsetup.sif
	$(CP) bootdata/icon.ico $(BOOTCD_DIR)/icon.ico
	$(CP) media/nls/c_1252.nls $(BOOTCD_DIR)/reactos/c_1252.nls
	$(CP) media/nls/c_437.nls $(BOOTCD_DIR)/reactos/c_437.nls
	$(CP) media/nls/l_intl.nls $(BOOTCD_DIR)/reactos/l_intl.nls

bootcd_basic: all bootcd_directory_layout bootcd_bootstrap_files bootcd_install_before

bootcd_makecd:
	$(CABMAN) /C bootdata/packages/reactos.dff /L $(BOOTCD_DIR)/reactos /I
	$(CABMAN) /C bootdata/packages/reactos.dff /RC $(BOOTCD_DIR)/reactos/reactos.inf /L $(BOOTCD_DIR)/reactos /N
	- $(RM) $(BOOTCD_DIR)/reactos/reactos.inf
	$(TOOLS_PATH)/cdmake/cdmake -v -m -b $(BOOTCD_DIR)/../isoboot.bin $(BOOTCD_DIR) REACTOS ReactOS.iso

ubootcd_unattend:
	$(CP) bootdata/unattend.inf $(BOOTCD_DIR)/reactos/unattend.inf

bootcd: bootcd_basic bootcd_makecd

ubootcd: bootcd_basic ubootcd_unattend bootcd_makecd

registry: tools
	$(TOOLS_PATH)/mkhive/mkhive$(EXE_POSTFIX) bootdata $(INSTALL_DIR)/system32/config

.PHONY: all depends implib clean clean_before install dist freeldr bootcd_directory_layout \
bootcd_bootstrap_files bootcd_install_before bootcd_basic bootcd_makecd ubootcd_unattend bootcd


#
# System Applications
#
$(SYS_APPS): %:
	$(MAKE) -C subsys/system/$*

$(SYS_APPS:%=%_implib): %_implib:
	$(MAKE) -C subsys/system/$* implib

$(SYS_APPS:%=%_clean): %_clean:
	$(MAKE) -C subsys/system/$* clean

$(SYS_APPS:%=%_dist): %_dist:
	$(MAKE) -C subsys/system/$* dist

$(SYS_APPS:%=%_install): %_install:
	$(MAKE) -C subsys/system/$* install

$(SYS_APPS:%=%_bootcd): %_bootcd:
	$(MAKE) -C subsys/system/$* bootcd

.PHONY: $(SYS_APPS) $(SYS_APPS:%=%_implib) $(SYS_APPS:%=%_clean) $(SYS_APPS:%=%_install) $(SYS_APPS:%=%_dist) $(SYS_APPS:%=%_bootcd)

#
# System Services
#
$(SYS_SVC): %:
	$(MAKE) -C services/$*

$(SYS_SVC:%=%_depends): %_depends:
	$(MAKE) -C services/$* depends

$(SYS_SVC:%=%_implib): %_implib:
	$(MAKE) -C services/$* implib

$(SYS_SVC:%=%_clean): %_clean:
	$(MAKE) -C services/$* clean

$(SYS_SVC:%=%_dist): %_dist:
	$(MAKE) -C services/$* dist

$(SYS_SVC:%=%_install): %_install:
	$(MAKE) -C services/$* install

.PHONY: $(SYS_SVC) $(SYS_SVC:%=%_depends) $(SYS_SVC:%=%_implib) $(SYS_SVC:%=%_clean) $(SYS_SVC:%=%_install) $(SYS_SVC:%=%_dist)


#
# Applications
#
#
# Extra (optional system) Applications
#
$(APPS): %:
	$(MAKE) -C apps/$*

$(APPS:%=%_implib): %_implib:
	$(MAKE) -C apps/$* implib

$(APPS:%=%_clean): %_clean:
	$(MAKE) -C apps/$* clean

$(APPS:%=%_dist): %_dist:
	$(MAKE) -C apps/$* dist

$(APPS:%=%_install): %_install:
	$(MAKE) -C apps/$* install

.PHONY: $(APPS) $(APPS:%=%_implib) $(APPS:%=%_clean) $(APPS:%=%_install) $(APPS:%=%_dist)


#
# External ports and subsystem personalities
#
$(EXTERNALS): %:
	$(MAKE) -C $(ROOT_PATH)/$*

$(EXTERNALS:%=%_depends): %_depends:
	$(MAKE) -C $(ROOT_PATH)/$* depends

$(EXTERNALS:%=%_implib): %_implib:
	$(MAKE) -C $(ROOT_PATH)/$* implib

$(EXTERNALS:%=%_clean): %_clean:
	$(MAKE) -C $(ROOT_PATH)/$* clean

$(EXTERNALS:%=%_dist): %_dist:
	$(MAKE) -C $(ROOT_PATH)/$* dist

$(EXTERNALS:%=%_install): %_install:
	$(MAKE) -C $(ROOT_PATH)/$* install

.PHONY: $(EXTERNALS) $(EXTERNALS:%=%_depends) $(EXTERNALS:%=%_implib) $(EXTERNALS:%=%_clean) $(EXTERNALS:%=%_install) $(EXTERNALS:%=%_dist)


#
# Tools
#
tools:
	$(MAKE) -C tools

tools_implib:

tools_clean:
	$(MAKE) -C tools clean

tools_install:

tools_dist:

.PHONY: tools tools_implib tools_clean tools_install tools_dist


#
# Developer Kits
#
dk:
	$(RMKDIR) $(DK_PATH)
	$(RMKDIR) $(DDK_PATH)
	$(RMKDIR) $(DDK_PATH_LIB)
	$(RMKDIR) $(DDK_PATH_INC)
	$(RMKDIR) $(SDK_PATH)
	$(RMKDIR) $(SDK_PATH_LIB)
	$(RMKDIR) $(SDK_PATH_INC)
	$(RMKDIR) $(XDK_PATH)
	$(RMKDIR) $(XDK_PATH_LIB)
	$(RMKDIR) $(XDK_PATH_INC)

dk_implib:

# WARNING! Be very sure that there are no important files
#          in these directories before cleaning them!!!
dk_clean:
	$(RM) $(DDK_PATH_LIB)/*.a
# $(RM) $(DDK_PATH_INC)/*.h
	$(RMDIR) $(DDK_PATH_LIB)
#	$(RMDIR) $(DDK_PATH_INC)
	$(RM) $(SDK_PATH_LIB)/*.a
# $(RM) $(SDK_PATH_INC)/*.h
	$(RMDIR) $(SDK_PATH_LIB)
#	$(RMDIR) $(SDK_PATH_INC)
	$(RM) $(XDK_PATH_LIB)/*.a
#	$(RM) $(XDK_PATH_INC)/*.h
	$(RMDIR) $(XDK_PATH_LIB)
#	$(RMDIR) $(XDK_PATH_INC)

dk_install:

dk_dist:

.PHONY: dk dk_implib dk_clean dk_install dk_dist


#
# Interfaces
#
iface_native:
	$(MAKE) -C iface/native

iface_native_implib:

iface_native_clean:
	$(MAKE) -C iface/native clean

iface_native_install:

iface_native_dist:

iface_native_bootcd:

iface_additional:
	$(MAKE) -C iface/addsys

iface_additional_implib:

iface_additional_clean:
	$(MAKE) -C iface/addsys clean

iface_additional_install:

iface_additional_dist:

iface_additional_bootcd:

.PHONY: iface_native iface_native_implib iface_native_clean iface_native_install \
        iface_native_dist iface_native_bootcd \
        iface_additional iface_additional_implib iface_additional_clean \
        iface_additional_install iface_additional_dist iface_additional_bootcd

#
# Bus driver rules
#
$(BUS): %:
	$(MAKE) -C drivers/bus/$*

$(BUS:%=%_implib): %_implib:
	$(MAKE) -C drivers/bus/$* implib

$(BUS:%=%_clean): %_clean:
	$(MAKE) -C drivers/bus/$* clean

$(BUS:%=%_install): %_install:
	$(MAKE) -C drivers/bus/$* install

$(BUS:%=%_dist): %_dist:
	$(MAKE) -C drivers/bus/$* dist

$(BUS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/bus/$* bootcd

.PHONY: $(BUS) $(BUS:%=%_implib) $(BUS:%=%_clean) \
        $(BUS:%=%_install) $(BUS:%=%_dist) $(BUS:%=%_bootcd)

#
# Driver support libraries rules
#
$(DRIVERS_LIB): %:
	$(MAKE) -C drivers/lib/$*

$(DRIVERS_LIB:%=%_implib): %_implib:
	$(MAKE) -C drivers/lib/$* implib

$(DRIVERS_LIB:%=%_clean): %_clean:
	$(MAKE) -C drivers/lib/$* clean

$(DRIVERS_LIB:%=%_install): %_install:
	$(MAKE) -C drivers/lib/$* install

$(DRIVERS_LIB:%=%_dist): %_dist:
	$(MAKE) -C drivers/lib/$* dist

$(DRIVERS_LIB:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/lib/$* bootcd

.PHONY: $(DRIVERS_LIB) $(DRIVERS_LIB:%=%_implib) $(DRIVERS_LIB:%=%_clean) \
        $(DRIVERS_LIB:%=%_install) $(DRIVERS_LIB:%=%_dist) $(DRIVERS_LIB:%=%_bootcd)

#
# Device driver rules
#
$(DEVICE_DRIVERS): %:
	$(MAKE) -C drivers/dd/$*

$(DEVICE_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/dd/$* implib

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/dd/$* clean

$(DEVICE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/dd/$* install

$(DEVICE_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/dd/$* dist

$(DEVICE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/dd/$* bootcd

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_implib) $(DEVICE_DRIVERS:%=%_clean) \
        $(DEVICE_DRIVERS:%=%_install) $(DEVICE_DRIVERS:%=%_dist) $(DEVICE_DRIVERS:%=%_bootcd)

#
# Input driver rules
#
$(INPUT_DRIVERS): %:
	$(MAKE) -C drivers/input/$*

$(INPUT_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/input/$* implib

$(INPUT_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/input/$* clean

$(INPUT_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/input/$* install

$(INPUT_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/input/$* dist

$(INPUT_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/input/$* bootcd

.PHONY: $(INPUT_DRIVERS) $(INPUT_DRIVERS:%=%_implib) $(INPUT_DRIVERS:%=%_clean)\
        $(INPUT_DRIVERS:%=%_install) $(INPUT_DRIVERS:%=%_dist) $(INPUT_DRIVERS:%=%_bootcd)

$(FS_DRIVERS): %:
	$(MAKE) -C drivers/fs/$*

$(FS_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/fs/$* implib

$(FS_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/fs/$* clean

$(FS_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/fs/$* install

$(FS_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/fs/$* dist

$(FS_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/fs/$* bootcd

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_implib) $(FS_DRIVERS:%=%_clean) \
        $(FS_DRIVERS:%=%_install) $(FS_DRIVERS:%=%_dist) $(FS_DRIVERS:%=%_bootcd)

#
# Network driver rules
#
$(NET_DRIVERS): %:
	$(MAKE) -C drivers/net/$*

$(NET_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/net/$* implib

$(NET_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/net/$* clean

$(NET_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/net/$* install

$(NET_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/net/$* dist

$(NET_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/net/$* bootcd

.PHONY: $(NET_DRIVERS) $(NET_DRIVERS:%=%_implib) $(NET_DRIVERS:%=%_clean) \
        $(NET_DRIVERS:%=%_install) $(NET_DRIVERS:%=%_dist) $(NET_DRIVERS:%=%_bootcd)

$(NET_DEVICE_DRIVERS): %:
	$(MAKE) -C drivers/net/dd/$*

$(NET_DEVICE_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/net/dd/$* implib

$(NET_DEVICE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/net/dd/$* clean

$(NET_DEVICE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/net/dd/$* install

$(NET_DEVICE_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/net/dd/$* dist

$(NET_DEVICE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/net/dd/$* bootcd

.PHONY: $(NET_DEVICE_DRIVERS) $(NET_DEVICE_DRIVERS:%=%_clean) $(NET_DEVICE_DRIVERS:%=%_implib) \
        $(NET_DEVICE_DRIVERS:%=%_install) $(NET_DEVICE_DRIVERS:%=%_dist) $(NET_DEVICE_DRIVERS:%=%_bootcd)

#
# storage driver rules
#
$(STORAGE_DRIVERS): %:
	$(MAKE) -C drivers/storage/$*

$(STORAGE_DRIVERS:%=%_implib): %_implib:
	$(MAKE) -C drivers/storage/$* implib

$(STORAGE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/storage/$* clean

$(STORAGE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/storage/$* install

$(STORAGE_DRIVERS:%=%_dist): %_dist:
	$(MAKE) -C drivers/storage/$* dist

$(STORAGE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/storage/$* bootcd

.PHONY: $(STORAGE_DRIVERS) $(STORAGE_DRIVERS:%=%_clean) $(STORAGE_DRIVERS:%=%_implib) \
		$(STORAGE_DRIVERS:%=%_install) $(STORAGE_DRIVERS:%=%_dist) $(STORAGE_DRIVERS:%=%_bootcd)

#
# Kernel loaders
#

$(LOADERS): %:
	$(MAKE) -C loaders/$*

$(LOADERS:%=%_implib): %_implib:

$(LOADERS:%=%_clean): %_clean:
	$(MAKE) -C loaders/$* clean

$(LOADERS:%=%_install): %_install:
	$(MAKE) -C loaders/$* install

$(LOADERS:%=%_dist): %_dist:
	$(MAKE) -C loaders/$* dist

.PHONY: $(LOADERS) $(LOADERS:%=%_implib) $(LOADERS:%=%_clean) $(LOADERS:%=%_install) \
        $(LOADERS:%=%_dist)

#
# Required system components
#

ntoskrnl:
	$(MAKE) -C ntoskrnl

ntoskrnl_implib:
	$(MAKE) -C ntoskrnl implib

ntoskrnl_clean:
	$(MAKE) -C ntoskrnl clean

ntoskrnl_install:
	$(MAKE) -C ntoskrnl install

ntoskrnl_dist:
	$(MAKE) -C ntoskrnl dist

ntoskrnl_bootcd:
	$(MAKE) -C ntoskrnl bootcd

.PHONY: ntoskrnl ntoskrnl_implib ntoskrnl_clean ntoskrnl_install ntoskrnl_dist ntoskrnl_bootcd

#
# Hardware Abstraction Layer import library
#

hallib:
	$(MAKE) -C hal/hal

hallib_implib:
	$(MAKE) -C hal/hal implib

hallib_clean:
	$(MAKE) -C hal/hal clean

hallib_install:
	$(MAKE) -C hal/hal install

hallib_dist:
	$(MAKE) -C hal/hal dist

hallib_bootcd:
	$(MAKE) -C hal/hal bootcd

.PHONY: hallib hallib_implib hallib_clean hallib_install hallib_dist hallib_bootcd

#
# Hardware Abstraction Layers
#

$(HALS): %:
	$(MAKE) -C hal/$*

$(HALS:%=%_implib): %_implib:
	$(MAKE) -C hal/$* implib

$(HALS:%=%_clean): %_clean:
	$(MAKE) -C hal/$* clean

$(HALS:%=%_install): %_install:
	$(MAKE) -C hal/$* install

$(HALS:%=%_dist): %_dist:
	$(MAKE) -C hal/$* dist

$(HALS:%=%_bootcd): %_bootcd:
	$(MAKE) -C hal/$* bootcd

.PHONY: $(HALS) $(HALS:%=%_implib) $(HALS:%=%_clean) $(HALS:%=%_install) $(HALS:%=%_dist) $(HALS:%=%_bootcd)

#
# File system libraries
#

$(LIB_FSLIB): %:
	$(MAKE) -C lib/fslib/$*

$(LIB_FSLIB:%=%_depends): %_depends:
	$(MAKE) -C lib/fslib/$* depends

$(LIB_FSLIB:%=%_implib): %_implib:
	$(MAKE) -C lib/fslib/$* implib

$(LIB_FSLIB:%=%_clean): %_clean:
	$(MAKE) -C lib/fslib/$* clean

$(LIB_FSLIB:%=%_install): %_install:
	$(MAKE) -C lib/fslib/$* install

$(LIB_FSLIB:%=%_dist): %_dist:
	$(MAKE) -C lib/fslib/$* dist

$(LIB_FSLIB:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/fslib/$* bootcd

.PHONY: $(LIB_FSLIB) $(LIB_FSLIB:%=%_depends) $(LIB_FSLIB:%=%_implib) $(LIB_FSLIB:%=%_clean) \
$(LIB_FSLIB:%=%_install) $(LIB_FSLIB:%=%_dist) $(LIB_FSLIB:%=%_bootcd)

#
# Static libraries
#

$(LIB_STATIC): %:
	$(MAKE) -C lib/$*

$(LIB_STATIC:%=%_depends): %_depends:
	$(MAKE) -C lib/string depends

$(LIB_STATIC:%=%_implib): %_implib:
	$(MAKE) -C lib/$* implib

$(LIB_STATIC:%=%_clean): %_clean:
	$(MAKE) -C lib/$* clean

$(LIB_STATIC:%=%_install): %_install:
	$(MAKE) -C lib/$* install

$(LIB_STATIC:%=%_dist): %_dist:
	$(MAKE) -C lib/$* dist

$(LIB_STATIC:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/$* bootcd

.PHONY: $(LIB_STATIC) $(LIB_STATIC:%=%_depends) $(LIB_STATIC:%=%_implib) $(LIB_STATIC:%=%_clean) \
	$(LIB_STATIC:%=%_install) $(LIB_STATIC:%=%_dist) $(LIB_STATIC:%=%_bootcd)

#
# Required DLLs
#

$(DLLS): %:
	$(MAKE) -C lib/$*

$(DLLS:%=%_depends): %_depends:
	$(MAKE) -C lib/$* depends

$(DLLS:%=%_implib): %_implib:
	$(MAKE) -C lib/$* implib

$(DLLS:%=%_clean): %_clean:
	$(MAKE) -C lib/$* clean

$(DLLS:%=%_install): %_install:
	$(MAKE) -C lib/$* install

$(DLLS:%=%_dist): %_dist:
	$(MAKE) -C lib/$* dist

$(DLLS:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/$* bootcd

.PHONY: $(DLLS) $(DLLS:%=%_depends) $(DLLS:%=%_implib) $(DLLS:%=%_clean) $(DLLS:%=%_install) $(DLLS:%=%_dist) $(DLLS:%=%_bootcd)

#
# Subsystem support modules
#

$(SUBSYS): %:
	$(MAKE) -C subsys/$*

$(SUBSYS:%=%_depends): %_depends:
	$(MAKE) -C subsys/$* depends

$(SUBSYS:%=%_implib): %_implib:
	$(MAKE) -C subsys/$* implib

$(SUBSYS:%=%_clean): %_clean:
	$(MAKE) -C subsys/$* clean

$(SUBSYS:%=%_install): %_install:
	$(MAKE) -C subsys/$* install

$(SUBSYS:%=%_dist): %_dist:
	$(MAKE) -C subsys/$* dist

$(SUBSYS:%=%_bootcd): %_bootcd:
	$(MAKE) -C subsys/$* bootcd

.PHONY: $(SUBSYS) $(SUBSYS:%=%_depends) $(SUBSYS:%=%_implib) $(SUBSYS:%=%_clean) $(SUBSYS:%=%_install) \
        $(SUBSYS:%=%_dist) $(SUBSYS:%=%_bootcd)

#
# Regression testsuite
#

$(REGTESTS): %:
	$(MAKE) -C regtests

$(REGTESTS:%=%_clean): %_clean:
	$(MAKE) -C regtests clean

$(REGTESTS:%=%_install): %_install:
	$(MAKE) -C regtests install

.PHONY: $(REGTESTS) $(REGTESTS:%=%_depends) $(SUBSYS:%=%_clean) $(REGTESTS:%=%_install)


#
# Create an installation
#

install_clean:
	$(RM) $(INSTALL_DIR)/system32/drivers/*.*
	$(RM) $(INSTALL_DIR)/system32/config/*.*
	$(RM) $(INSTALL_DIR)/system32/*.*
	$(RM) $(INSTALL_DIR)/symbols/*.*
	$(RM) $(INSTALL_DIR)/media/fonts/*.*
	$(RM) $(INSTALL_DIR)/media/*.*
	$(RM) $(INSTALL_DIR)/bin/*.*
	$(RM) $(INSTALL_DIR)/*.com
	$(RM) $(INSTALL_DIR)/*.bat
	$(RMDIR) $(INSTALL_DIR)/system32/drivers
	$(RMDIR) $(INSTALL_DIR)/system32/config
	$(RMDIR) $(INSTALL_DIR)/system32
	$(RMDIR) $(INSTALL_DIR)/symbols
	$(RMDIR) $(INSTALL_DIR)/media/fonts
	$(RMDIR) $(INSTALL_DIR)/media
	$(RMDIR) $(INSTALL_DIR)/bin
	$(RMDIR) $(INSTALL_DIR)

ifneq ($(BOOTCD_INSTALL),)

install_dirs:
	$(RMKDIR) $(INSTALL_DIR)

install_before:
	$(RLINE) bootdata/autorun.inf $(INSTALL_DIR)/../autorun.inf
	$(RLINE) bootdata/readme.txt $(INSTALL_DIR)/../readme.txt
	$(RLINE) bootdata/hivecls.inf $(INSTALL_DIR)/hivecls.inf
	$(RLINE) bootdata/hivedef.inf $(INSTALL_DIR)/hivedef.inf
	$(RLINE) bootdata/hivesft.inf $(INSTALL_DIR)/hivesft.inf
	$(RLINE) bootdata/hivesys.inf $(INSTALL_DIR)/hivesys.inf
	$(RLINE) bootdata/txtsetup.sif $(INSTALL_DIR)/txtsetup.sif
	$(CP) bootdata/icon.ico $(INSTALL_DIR)/../icon.ico
	$(CP) media/fonts $(INSTALL_DIR)
	$(CP) media/nls $(INSTALL_DIR)

else # !BOOTCD_INSTALL

install_dirs:
	$(RMKDIR) $(INSTALL_DIR)
	$(RMKDIR) $(INSTALL_DIR)/bin
	$(RMKDIR) $(INSTALL_DIR)/media
	$(RMKDIR) $(INSTALL_DIR)/media/fonts
	$(RMKDIR) $(INSTALL_DIR)/symbols
	$(RMKDIR) $(INSTALL_DIR)/system32
	$(RMKDIR) $(INSTALL_DIR)/system32/config
	$(RMKDIR) $(INSTALL_DIR)/system32/drivers

install_before:
	$(CP) bootc.lst $(INSTALL_DIR)/bootc.lst
	$(CP) boot.bat $(INSTALL_DIR)/boot.bat
	$(CP) aboot.bat $(INSTALL_DIR)/aboot.bat
	$(CP) media/fonts $(INSTALL_DIR)/media/fonts
	$(CP) media/nls $(INSTALL_DIR)/system32
	$(CP) media/nls/c_1252.nls $(INSTALL_DIR)/system32/ansi.nls
	$(CP) media/nls/c_437.nls $(INSTALL_DIR)/system32/oem.nls
	$(CP) media/nls/l_intl.nls $(INSTALL_DIR)/system32/casemap.nls

endif # BOOTCD_INSTALL

.PHONY: install_clean install_dirs install_before


#
# Make a distribution saveset
#

dist_clean:
	$(RM) $(DIST_DIR)/symbols/*.sym
	$(RM) $(DIST_DIR)/drivers/*.sys
	$(RM) $(DIST_DIR)/subsys/*.exe
	$(RM) $(DIST_DIR)/dlls/*.dll
	$(RM) $(DIST_DIR)/apps/*.exe
	$(RM) $(DIST_DIR)/*.exe
	$(RMDIR) $(DIST_DIR)/symbols
	$(RMDIR) $(DIST_DIR)/subsys
	$(RMDIR) $(DIST_DIR)/drivers
	$(RMDIR) $(DIST_DIR)/dlls
	$(RMDIR) $(DIST_DIR)/apps
	$(RMDIR) $(DIST_DIR)

dist_dirs:
	$(RMKDIR) $(DIST_DIR)
	$(RMKDIR) $(DIST_DIR)/apps
	$(RMKDIR) $(DIST_DIR)/dlls
	$(RMKDIR) $(DIST_DIR)/drivers
	$(RMKDIR) $(DIST_DIR)/subsys
	$(RMKDIR) $(DIST_DIR)/symbols

.PHONY: dist_clean dist_dirs

etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF


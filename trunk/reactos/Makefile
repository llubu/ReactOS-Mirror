#
# Global makefile
#

#
# Select your host
#
#HOST = mingw32-linux
#HOST = djgpp-msdos
#HOST = mingw32-windows

include rules.mak

#
# Required to run the system
#
COMPONENTS = iface_native ntoskrnl
DLLS = ntdll kernel32 crtdll advapi32 fmifs gdi32 psxdll
#DLLS = mingw32 user32
SUBSYS = smss win32k
#SUBSYS = csrss

#
# Select the server(s) you want to build
#
SERVERS = win32
# SERVERS = posix linux os2

#
# Select the loader(s) you want to build
#
LOADERS = dos
# LOADERS = boot

#
# Select the device drivers and filesystems you want
#
DEVICE_DRIVERS = blue ide keyboard null parallel serial
# DEVICE_DRIVERS = beep event floppy ide_test mouse sound test test1
FS_DRIVERS = vfat
# FS_DRIVERS = minix ext2 template
KERNEL_SERVICES = $(DEVICE_DRIVERS) $(FS_DRIVERS)

APPS = args hello shell test cat bench
# APPS = cmd

all: buildno $(COMPONENTS) $(DLLS) $(SUBSYS) $(LOADERS) $(KERNEL_SERVICES) $(APPS)

.PHONY: all

clean: buildno_clean $(COMPONENTS:%=%_clean) $(DLLS:%=%_clean) $(LOADERS:%=%_clean) \
       $(KERNEL_SERVICES:%=%_clean) $(SUBSYS:%=%_clean) $(APPS:%=%_clean)
       
.PHONY: clean

floppy: make_floppy_dirs autoexec_floppy $(COMPONENTS:%=%_floppy) \
        $(DLLS:%=%_floppy) $(LOADERS:%=%_floppy) \
        $(KERNEL_SERVICES:%=%_floppy) $(SUBSYS:%=%_floppy) \
        $(APPS:%=%_floppy)

dist: clean_dist_dir make_dist_dirs $(COMPONENTS:%=%_dist) $(DLLS:%=%_dist) \
      $(LOADERS:%=%_dist) $(KERNEL_SERVICES:%=%_dist) $(SUBSYS:%=%_dist) \
      $(APPS:%=%_dist)

#
# Build number generator
#
buildno: include/reactos/version.h
	make -C apps/buildno

buildno_clean:
	make -C apps/buildno clean

buildno_floppy:

buildno_dist:

.PHONY: buildno buildno_clean buildno_floppy buildno_dist

#
# Applications
#
$(APPS): %:
	make -C apps/$*

$(APPS:%=%_clean): %_clean:
	make -C apps/$* clean

$(APPS:%=%_floppy): %_floppy:
	make -C apps/$* floppy

$(APPS:%=%_dist): %_dist:
	make -C apps/$* dist

.PHONY: $(APPS) $(APPS:%=%_clean) $(APPS:%=%_floppy) $(APPS:%=%_dist)

#
# Interfaces
#
iface_native:
	make -C iface/native

iface_native_clean:
	make -C iface/native clean

iface_native_floppy:

iface_native_dist:

.PHONY: iface_native iface_native_clean iface_native_floppy \
        iface_native_dist

#
# Device driver rules
#
$(DEVICE_DRIVERS): %:
	make -C services/dd/$*

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C services/dd/$* clean

$(DEVICE_DRIVERS:%=%_floppy): %_floppy:
	make -C services/dd/$* floppy

$(DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C services/dd/$* dist

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_clean) \
        $(DEVICE_DRIVERS:%=%_floppy) $(DEVICE_DRIVERS:%=%_dist)

$(FS_DRIVERS): %:
	make -C services/fs/$*

$(FS_DRIVERS:%=%_clean): %_clean:
	make -C services/fs/$* clean

$(FS_DRIVERS:%=%_floppy): %_floppy:
	make -C services/fs/$* floppy

$(FS_DRIVERS:%=%_dist): %_dist:
	make -C services/fs/$* dist

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_clean) $(FS_DRIVERS:%=%_floppy) \
        $(FS_DRIVERS:%=%_dist)

#
# Kernel loaders
#

$(LOADERS): %:
	make -C loaders/$*

$(LOADERS:%=%_clean): %_clean:
	make -C loaders/$* clean

$(LOADERS:%=%_floppy): %_floppy:
	make -C loaders/$* floppy

$(LOADERS:%=%_dist): %_dist:
	make -C loaders/$* dist

.PHONY: $(LOADERS) $(LOADERS:%=%_clean) $(LOADERS:%=%_floppy) \
        $(LOADERS:%=%_dist)

#
# Required system components
#

ntoskrnl:
	make -C ntoskrnl

ntoskrnl_clean:
	make -C ntoskrnl clean

ntoskrnl_floppy:
	make -C ntoskrnl floppy

ntoskrnl_dist:
	make -C ntoskrnl dist

.PHONY: ntoskrnl ntoskrnl_clean ntoskrnl_floppy ntoskrnl_dist

#
# Required DLLs
#

$(DLLS): %:
	make -C lib/$*

$(DLLS:%=%_clean): %_clean:
	make -C lib/$* clean

$(DLLS:%=%_floppy): %_floppy:
	make -C lib/$* floppy

$(DLLS:%=%_dist): %_dist:
	make -C lib/$* dist

.PHONY: $(DLLS) $(DLLS:%=%_clean) $(DLLS:%=%_floppy) $(DLLS:%=%_dist)

#
# Kernel Subsystems
#
$(SUBSYS): %:
	make -C subsys/$*

$(SUBSYS:%=%_clean): %_clean:
	make -C subsys/$* clean

$(SUBSYS:%=%_floppy): %_floppy:
	make -C subsys/$* floppy

$(SUBSYS:%=%_dist): %_dist:
	make -C subsys/$* dist

.PHONY: $(SUBSYS) $(SUBSYS:%=%_clean) $(SUBSYS:%=%_floppy) \
        $(SUBSYS:%=%_dist)

#
# Make an install floppy
#

install: all
	./install.sh /mnt/hda1

make_floppy_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(FLOPPY_DIR)\dlls
	mkdir $(FLOPPY_DIR)\apps
	mkdir $(FLOPPY_DIR)\drivers
	mkdir $(FLOPPY_DIR)\subsys
else
	mkdir $(FLOPPY_DIR)/dlls $(FLOPPY_DIR)/apps $(FLOPPY_DIR)/drivers
	mkdir $(FLOPPY_DIR)/subsys
endif

.PHONY: make_floppy_dirs

autoexec_floppy: $(FLOPPY_DIR)/autoexec.bat

$(FLOPPY_DIR)/autoexec.bat: bootflop.bat
ifeq ($(DOSCLI),yes)
	$(CP) bootflop.bat $(FLOPPY_DIR)\autoexec.bat
else
	$(CP) bootflop.bat $(FLOPPY_DIR)/autoexec.bat
endif

#
# Make a distribution saveset
#

clean_dist_dir:
ifeq ($(DOSCLI),yes)
	$(RM) $(DIST_DIR)\dlls\*.*
	$(RM) $(DIST_DIR)\apps\*.*
	$(RM) $(DIST_DIR)\drivers\*.*
	$(RM) $(DIST_DIR)\subsys\*.*
	$(RMDIR) $(DIST_DIR)\dlls
	$(RMDIR) $(DIST_DIR)\apps
	$(RMDIR) $(DIST_DIR)\drivers
	$(RMDIR) $(DIST_DIR)\subsys
	$(RMDIR) $(DIST_DIR)
else
	$(RM) -r $(DIST_DIR)
endif

make_dist_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)\dlls
	mkdir $(DIST_DIR)\apps
	mkdir $(DIST_DIR)\drivers
	mkdir $(DIST_DIR)\dlls
	mkdir $(DIST_DIR)\subsys
else
	mkdir $(DIST_DIR) $(DIST_DIR)/dlls $(DIST_DIR)/apps $(DIST_DIR)/drivers
	mkdir $(DIST_DIR)/subsys
endif

.PHONY: clean_dist_dir make_dist_dirs



ROOTDIR = ../../..
VMDIR = ../..

IMAGE = SqueakNOS
STORAGE = iso

include $(ROOTDIR)/nopsys/compilation.conf
include plugins

#CGLAGS += -mno-omit-leaf-frame-pointer ???
# -ffreestanding
CFLAGS = $(CFLAGS_ARCH) -Wall -nostdinc -nostdlib -std=gnu11\
			-msse2 -fno-inline -fno-omit-frame-pointer -r\
			-Wno-unknown-pragmas -Wno-missing-braces -Wno-unused-value -Wno-unused-label -Wno-unused-function -Wno-unused-but-set-variable  


DEFS  = -DNDEBUG -DDEBUGVM=0 -DPharoVM -DLSB_FIRST=1 -DITIMER_HEARTBEAT=1 $(DEFS_ARCH) $(XDEFS)


CROSSDIR  = $(VMDIR)/platforms/Cross
NOPSYSDIR = $(VMDIR)/platforms/nopsys

VM_BUILDDIR=../opensmalltalk-vm/platforms/nopsys/build

OPENLIBMDIR = $(VMDIR)/third-party/openlibm
OPENLIBM = $(OPENLIBMDIR)/libopenlibm.a

#VM=jit
VM=interpreter


CROSS_C_FILES  = $(wildcard $(CROSSDIR)/vm/*.c)
ifdef JIT
	COGDIR    = $(VMDIR)/spur64src/vm
	COG_C_FILES    = $(COGDIR)/cogit.c $(COGDIR)/gcc3x-cointerp.c
	DEFS+= -DNOPSYS_COGVM -DCOGMTVM=0 -g -O1
else
	COGDIR    = $(VMDIR)/spurstack64src/vm
	COG_C_FILES    = $(COGDIR)/gcc3x-interp.c
	DEFS+= -g -O1
endif


NOPSYS_C_FILES = $(wildcard $(NOPSYSDIR)/*.c)

ALLPLUGIN_OBJS = $(addsuffix .pluginobj, $(INTERNAL_PLUGINS)) 


VMSRC  = $(filter-out interp.c, $(CROSS_C_FILES) $(NOPSYS_C_FILES) $(COG_C_FILES))
VMOBJS = $(notdir $(VMSRC:.c=.o)) $(ALLPLUGIN_OBJS)
VMOBJS_BUILD = $(addprefix build/, $(VMOBJS))

# PLUGIN variable is defined externally, when calling make (for a specific plugin)

INCS =-I$(CROSSDIR)/plugins/$(PLUGIN) -I$(CROSSDIR)/vm -I$(COGDIR) -I$(NOPSYSDIR) -I$(ROOTDIR)/nopsys/src -I$(ROOTDIR)/nopsys/src/libc

ifeq ($(PLUGIN),SqueakFFIPrims)
	ALL_PLUGIN_C_FILES = $(wildcard  ../../src/plugins/$(PLUGIN)/X64SysVFFIPlugin.c) $(wildcard $(CROSSDIR)/plugins/$(PLUGIN)/*.c)
else
	ALL_PLUGIN_C_FILES = $(wildcard ../../src/plugins/$(PLUGIN)/*.c) $(wildcard $(CROSSDIR)/plugins/$(PLUGIN)/*.c)
endif
PLUGIN_C_FILES = $(filter-out $(CROSSDIR)/plugins/$(PLUGIN)/BitBltArm%, $(ALL_PLUGIN_C_FILES))
PLUGIN_OBJS = $(notdir $(PLUGIN_C_FILES:.c=.o))
PLUGIN_OBJS_BUILD = $(addprefix build/, $(PLUGIN_OBJS))

# setup make path so that it finds .c files
VPATH = $(COGDIR):$(CROSSDIR)/vm:$(NOPSYSDIR):$(VMDIR)/src/plugins/$(PLUGIN):$(CROSSDIR)/plugins/$(PLUGIN)

.PHONY: iso

build/vm.obj: build sqNamedPrims.h $(VMOBJS_BUILD) $(INTERNAL_LIBS) $(OPENLIBM)
	$(CC) -o build/vm.obj $(CFLAGS) $(VMOBJS_BUILD) $(INTERNAL_LIBS) $(OPENLIBM)
#$(LDFLAGS)

build/extra-files:
	printf "../image/" > build/extra-files
	$(FIND) $(ROOTDIR)/image -name PharoV60.sources -printf "%P" >>build/extra-files
# find adds an enter in osx even if downloading the unix find from findutils	
#printf "pharo-vm/PharoV60.sources" >> build/extra-files
	printf " ../image/$(IMAGE).image ../image/$(IMAGE).changes" >>build/extra-files

sqNamedPrims.h: plugins 
	./gen_plugins_file.sh $(INTERNAL_PLUGINS_EXPORTS)
	rm -f $(VM_BUILDDIR)/sqNamedPrims.o

build/%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $(INCS) $(DEFS) $<

# to make a plugin, we call ourselves recursively, specifying which files have to be compiled
build/%.pluginobj:
	@$(MAKE) XDEFS=-DSQUEAK_BUILTIN_PLUGIN PLUGIN=$* plugin

$(OPENLIBM):
	make USEGCC=1 LOCALLIBC=../../../nopsys/src/libc/ -C $(OPENLIBMDIR)

plugin: $(PLUGIN_OBJS_BUILD) 
	$(AR) rc build/$(PLUGIN).pluginobj $(PLUGIN_OBJS_BUILD)
	$(RM) $(PLUGIN_OBJS_BUILD)

iso: build/vm.obj build/extra-files
	export VM_BUILDDIR=$(VM_BUILDDIR) && make -C $(ROOTDIR)/nopsys $@

hd: build/vm.obj build/extra-files
	export VM_BUILDDIR=$(VM_BUILDDIR) && make -C $(ROOTDIR)/nopsys $@

try-%: $(STORAGE)
	export VM_BUILDDIR=$(VM_BUILDDIR) STORAGE=$(STORAGE) && make -C $(ROOTDIR)/nopsys $@ 

clean:
	rm -rf build/*
	make -C $(ROOTDIR)/nopsys clean

build:
	mkdir -p build



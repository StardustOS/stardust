#  Copyright (C) 2020, Ward Jaradat
 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

XEN_ROOT = $(realpath ../../xen)

include $(XEN_ROOT)/Config.mk
export XEN_ROOT
XEN_INTERFACE_VERSION := 0x00030205
export XEN_INTERFACE_VERSION

ARCH_CFLAGS := -m64 -mno-red-zone -fPIC -fno-reorder-blocks
ARCH_CFLAGS += -fno-asynchronous-unwind-tables
ARCH_CFLAGS += -ffixed-r14
ARCH_ASFLAGS := -m64
ARCH_LDFLAGS := -m elf_x86_64

GCC_BASE=$(shell $(CC) -print-search-dirs | grep ^install | cut -f 2 -d ' ')
GCC_INCLUDE:=-I${GCC_BASE}include
GCC_INCLUDE+=-I${GCC_BASE}include-fixed

DEF_CFLAGS := -fno-builtin -nostdinc $(GCC_INCLUDE)
DEF_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)
DEF_CFLAGS += -Wall 
DEF_CFLAGS += -Wredundant-decls
DEF_CFLAGS += -Wno-format
DEF_CFLAGS += -fno-stack-protector
DEF_CFLAGS += -Wno-unused-but-set-variable
DEF_CFLAGS += -Wno-unused-variable
DEF_CFLAGS += -Wno-maybe-uninitialized
DEF_CFLAGS += -O3
DEF_CFLAGS += -mfpmath=sse
DEF_CFLAGS += -fomit-frame-pointer
DEF_CFLAGS += -Wno-int-conversion
DEF_CFLAGS += -Wno-pointer-arith
DEF_CFLAGS += -Wnested-externs -Wpointer-arith -Winline -Wno-parentheses -Wno-misleading-indentation
DEF_CFLAGS += -D__XEN_INTERFACE_VERSION__=$(XEN_INTERFACE_VERSION)
DEF_CFLAGS += -DCONFIG_PREEMPT -DCONFIG_SMP
DEF_CFLAGS += $(STARDUST_CFLAG)

DEF_ASFLAGS = -D__ASSEMBLY__
DEF_LDFLAGS = -no-stack-protector
DEF_LDFLAGS += --allow-multiple-definition

ifeq ($(debug),y)
DEF_CFLAGS += -g
endif

CFLAGS := $(DEF_CFLAGS) $(ARCH_CFLAGS)

ASFLAGS := $(DEF_ASFLAGS) $(ARCH_ASFLAGS)

LDFLAGS := $(DEF_LDFLAGS) $(ARCH_LDFLAGS) 

SRC_ROOT=$(PWD)
export SRC_ROOT

ARCH_INC := $(SRC_ROOT)/include/x86

HDRS := $(wildcard $(SRC_ROOT)/include/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/xen/*.h)
HDRS += $(wildcard $(ARCH_INC)/include/os/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/asm/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/sys/*.h)
HDRS += $(wildcard $(ARCH_INC)/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/arch/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/pte/*.h)
HDRS += $(wildcard $(SRC_ROOT)/include/math/*.h)

override CPPFLAGS := -I$(SRC_ROOT)/include $(CPPFLAGS) 

%.o: %.c $(HDRS) Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.S $(HDRS) Makefile
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

%.E: %.c $(HDRS) Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -E $< -o $@

ARCH_LIB_NAME = x86_64
ARCH_LIB := lib$(ARCH_LIB_NAME).a
LDFLAGS_FINAL := -N -T arch/loader.lds

TARGET := stardust

SUBDIRS := xenbus util atomic lib lib/asm
SUBDIRS += lib/pte lib/pte/platform lib/pte/posix lib/pte/posix/extra lib/pte/tls lib/pte/tests

ARCH_OBJS := $(patsubst %.c,%.o,$(wildcard arch/*.c))
ARCH_OBJS += $(patsubst %.S,%.o,$(wildcard arch/*.S))

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard atomic/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard xenbus/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/tls/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/posix/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/posix/extra/*.c))
OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/platform/*.c))
# OBJS += $(patsubst %.c,%.o,$(wildcard lib/pte/tests/*.c))
OBJS += $(patsubst %.S,%.o,$(wildcard lib/asm/*.S))

.PHONY: default
default: links archlib $(TARGET) 

.PHONY: links
links:	$(ARCH_LINKS) 
	[ -e include/public ] || ln -sf $(XEN_ROOT)/xen/include/public include/public


archlib: $(ARCH_OBJS) 
	$(AR) rv $(ARCH_LIB) $(ARCH_OBJS)

$(TARGET): $(OBJS)
	$(LD) -r $(LDFLAGS) arch/x86_64.o $(OBJS) -Larch $(ARCH_LIB) -o $@.o
	$(OBJCOPY) -w -G _start -G str* -G mem* -G hypercall_page $@.o $@.o
	$(LD) $(LDFLAGS) $(LDFLAGS_FINAL) $@.o -o $@
	gzip -f -9 -c $@ >$@.gz

.PHONY: clean arch_clean

arch_clean:
	rm -f $(ARCH_LIB) $(ARCH_OBJS)

clean:	arch_clean
	for dir in $(SUBDIRS); do \
		rm -f $$dir/*.o; \
	done
	rm -f *.o *~ core $(TARGET).elf $(TARGET).raw $(TARGET) $(TARGET).gz
	find . -type l | xargs rm -f

dump:
	$(MKDIR) $(PROJECT_DEBUG_DIR)
	$(CURRENT_DATE_AND_TIME)
	$(READELF) --debug-dump=decodedline $(TARGET) > $(TARGET)-info.txt
	$(OBJDUMP) -S $(TARGET) > $(TARGET)-dump.txt

%.S: %.c 
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $< -o $@
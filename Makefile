TITLE       := ORBIT Command Center
VERSION     := 0.1.0
TITLE_ID    := ORBT00001
CONTENT_ID  := IV0000-ORBT00001_00-ORBITFOUNDATION

SDK         ?= $(OO_PS4_TOOLCHAIN)
ifeq ($(strip $(SDK)),)
$(error Set OO_PS4_TOOLCHAIN or pass SDK=/path/to/OpenOrbis/SDK)
endif

CC          := clang
CXX         := clang++
LD          := ld.lld
HOST_OS     := $(shell uname -s)
ifeq ($(HOST_OS),Linux)
TOOL_DIR    := $(SDK)/bin/linux
else ifeq ($(HOST_OS),Darwin)
TOOL_DIR    := $(SDK)/bin/macos
else
$(error Unsupported host OS: $(HOST_OS))
endif

SOURCES     := src/main.cpp src/platform/video_output.cpp src/graphics/canvas.cpp
OBJECTS     := $(patsubst src/%.cpp,build/obj/%.o,$(SOURCES))
CXXFLAGS    := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -fno-exceptions -fno-rtti -O2 -c -Isrc -Iinclude -isysroot $(SDK) -isystem $(SDK)/include -isystem $(SDK)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(SDK)/link.x --eh-frame-hdr -L$(SDK)/lib -lc -lkernel -lc++ -lSceVideoOut $(SDK)/lib/crt1.o
DOTNET_ENV  := DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

.PHONY: all clean package inspect
all: build/$(CONTENT_ID).pkg

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<

build/orbit.elf: $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(LDFLAGS)

eboot.bin: build/orbit.elf
	$(TOOL_DIR)/create-fself -in=$< -out=build/orbit.oelf --eboot "$@" --paid 0x3800000000000011

sce_sys/param.sfo: Makefile
	@mkdir -p sce_sys
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_new $@
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx
	$(TOOL_DIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

build/$(CONTENT_ID).pkg: pkg.gp4
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core pkg_build $< build

package: all

inspect: build/$(CONTENT_ID).pkg
	$(DOTNET_ENV) $(TOOL_DIR)/PkgTool.Core pkg_listentries $<

clean:
	rm -rf build eboot.bin pkg.gp4 sce_sys/param.sfo

TITLE       := ORBIT Game Library
VERSION     := 0.5.0
TITLE_ID    := ORBT00001
CONTENT_ID  := IV0000-ORBT00001_00-ORBITFOUNDATION1

SDK         ?= $(OO_PS4_TOOLCHAIN)
ifeq ($(strip $(SDK)),)
$(error Set OO_PS4_TOOLCHAIN or pass SDK=/path/to/OpenOrbis/SDK)
endif

DOTNET_ROOT       ?= $(HOME)/.dotnet
DOTNET            := $(DOTNET_ROOT)/dotnet
LIBORBISPKG_ROOT  ?= $(abspath $(SDK)/../Tools/LibOrbisPkg)
LIBORBISPKG_REV   := $(shell git -C "$(LIBORBISPKG_ROOT)" rev-parse HEAD 2>/dev/null)
PKGTOOL_SOURCE    := build/tools/source
PKGTOOL_PROJECT   := $(PKGTOOL_SOURCE)/PkgTool.Core/PkgTool.Core.csproj
PKGTOOL_STAMP     := $(PKGTOOL_SOURCE)/.source-$(LIBORBISPKG_REV)
PKGTOOL           := build/tools/publish/PkgTool.Core
DOTNET_ENV        := DOTNET_ROOT="$(DOTNET_ROOT)" PATH="$(DOTNET_ROOT):$(PATH)"

ifeq ($(strip $(LIBORBISPKG_REV)),)
$(error Could not find a LibOrbisPkg git checkout at $(LIBORBISPKG_ROOT); set LIBORBISPKG_ROOT)
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

SOURCES     := src/main.cpp src/platform/video_output.cpp src/platform/controller.cpp src/platform/title_launcher.cpp src/graphics/canvas.cpp src/graphics/font_renderer.cpp src/graphics/texture.cpp src/ui/ui.cpp src/input/navigation.cpp src/library/catalog.cpp src/library/preferences.cpp src/modules/dashboard.cpp
OBJECTS     := $(patsubst src/%.cpp,build/obj/%.o,$(SOURCES))
CXXFLAGS    := --target=x86_64-pc-freebsd12-elf -DORBIT_PS4 -fPIC -funwind-tables -fno-exceptions -fno-rtti -O2 -c -Isrc -Iinclude -isysroot $(SDK) -isystem $(SDK)/include -isystem $(SDK)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(SDK)/link.x --eh-frame-hdr -L$(SDK)/lib -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lSceFreeType -lScePad -lSceUserService -lSceLncUtil $(SDK)/lib/crt1.o
ASSETS      := $(shell find assets -type f ! -name .gitkeep -print | sort)
HOST_CXX    ?= c++

.PHONY: all clean package inspect test-host
all: build/$(CONTENT_ID).pkg

$(PKGTOOL_STAMP):
	@test -f "$(LIBORBISPKG_ROOT)/PkgTool.Core/PkgTool.Core.csproj" || { echo "Missing PkgTool.Core.csproj under $(LIBORBISPKG_ROOT)"; exit 1; }
	@mkdir -p $(PKGTOOL_SOURCE)
	git -C "$(LIBORBISPKG_ROOT)" archive --format=tar HEAD | tar -xf - -C $(PKGTOOL_SOURCE)
	@printf '%s\n' '$(LIBORBISPKG_REV)' > $@

$(PKGTOOL): $(PKGTOOL_STAMP)
	$(DOTNET_ENV) $(DOTNET) publish "$(PKGTOOL_PROJECT)" -r linux-x64 --self-contained true -p:PublishSingleFile=true -p:NuGetAudit=false -o ./build/tools/publish
	@test -x $@

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(patsubst %.o,%.d,$@) -MT $@ -o $@ $<

-include $(OBJECTS:.o=.d)

build/orbit.elf: $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(LDFLAGS)

eboot.bin: build/orbit.elf
	$(TOOL_DIR)/create-fself -in=$< -out=build/orbit.oelf --eboot "$@" --paid 0x3800000000000011

sce_sys/param.sfo: Makefile $(PKGTOOL)
	@mkdir -p sce_sys
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx sce_module/libc.prx sce_module/libSceFios2.prx $(ASSETS)
	$(TOOL_DIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

build/$(CONTENT_ID).pkg: pkg.gp4 $(PKGTOOL)
	$(PKGTOOL) pkg_build $< build

package: all

inspect: build/$(CONTENT_ID).pkg $(PKGTOOL)
	$(PKGTOOL) pkg_listentries $<
	$(PKGTOOL) pkg_validate $<

build/tests/canvas_test: tests/canvas_test.cpp src/graphics/canvas.cpp include/orbit/graphics/canvas.hpp
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -o $@ tests/canvas_test.cpp src/graphics/canvas.cpp

build/tests/font_renderer_test: tests/font_renderer_test.cpp src/graphics/font_renderer.cpp src/graphics/canvas.cpp include/orbit/graphics/font_renderer.hpp assets/fonts/Quicksand-Medium.ttf
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -idirafter $(SDK)/include $(shell pkg-config --cflags freetype2) -o $@ tests/font_renderer_test.cpp src/graphics/font_renderer.cpp src/graphics/canvas.cpp $(shell pkg-config --libs freetype2)

build/tests/texture_test: tests/texture_test.cpp src/graphics/texture.cpp src/graphics/canvas.cpp include/orbit/graphics/texture.hpp assets/images/orbit-mark.png
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -idirafter $(SDK)/include -o $@ tests/texture_test.cpp src/graphics/texture.cpp src/graphics/canvas.cpp

build/tests/navigation_test: tests/navigation_test.cpp src/input/navigation.cpp include/orbit/input/controller.hpp
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -o $@ tests/navigation_test.cpp src/input/navigation.cpp

build/tests/ui_test: tests/ui_test.cpp src/ui/ui.cpp src/graphics/font_renderer.cpp src/graphics/texture.cpp src/graphics/canvas.cpp include/orbit/ui/ui.hpp assets/fonts/Quicksand-Medium.ttf
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -idirafter $(SDK)/include $(shell pkg-config --cflags freetype2) -o $@ tests/ui_test.cpp src/ui/ui.cpp src/graphics/font_renderer.cpp src/graphics/texture.cpp src/graphics/canvas.cpp $(shell pkg-config --libs freetype2)

build/tests/dashboard_test: tests/dashboard_test.cpp src/modules/dashboard.cpp src/platform/title_launcher.cpp src/library/catalog.cpp src/library/preferences.cpp src/ui/ui.cpp src/input/navigation.cpp src/graphics/font_renderer.cpp src/graphics/texture.cpp src/graphics/canvas.cpp include/orbit/modules/dashboard.hpp assets/fonts/Quicksand-Medium.ttf assets/images/orbit-mark.png
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -idirafter $(SDK)/include $(shell pkg-config --cflags freetype2) -o $@ tests/dashboard_test.cpp src/modules/dashboard.cpp src/platform/title_launcher.cpp src/library/catalog.cpp src/library/preferences.cpp src/ui/ui.cpp src/input/navigation.cpp src/graphics/font_renderer.cpp src/graphics/texture.cpp src/graphics/canvas.cpp $(shell pkg-config --libs freetype2)

build/tests/library_test: tests/library_test.cpp src/library/catalog.cpp src/library/preferences.cpp include/orbit/library/game.hpp include/orbit/library/preferences.hpp
	@mkdir -p $(dir $@)
	$(HOST_CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude -o $@ tests/library_test.cpp src/library/catalog.cpp src/library/preferences.cpp

.PHONY: test-host
test-host: build/tests/canvas_test build/tests/font_renderer_test build/tests/texture_test build/tests/navigation_test build/tests/ui_test build/tests/dashboard_test build/tests/library_test
	./build/tests/canvas_test
	./build/tests/font_renderer_test
	./build/tests/texture_test
	./build/tests/navigation_test
	./build/tests/ui_test
	./build/tests/dashboard_test
	./build/tests/library_test

clean:
	rm -rf build eboot.bin pkg.gp4 sce_sys/param.sfo

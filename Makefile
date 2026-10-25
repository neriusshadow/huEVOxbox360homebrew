# huEVO Xbox 360 - OXDK Makefile
# Targets Xbox 360 (XEX2 executable via LLVM + MSVC XDK libs)

OXDK_DIR        ?= /Users/neriusneriusnerius/OXDK
OXDK_TARGET     := xbox360
OXDK360_LIBCXX  := 1

XEX_TITLE   := default
XEX_BASE    := 0x82000000
XEX_STACK   := 0x100000
XENON_ABI   := 1

SRCS := \
    src/main.cpp \
    src/app.cpp \
    src/renderer.cpp \
    src/font.cpp \
    src/input.cpp \
    src/network.cpp \
    src/keyboard.cpp \
    src/ui.cpp

CXXFLAGS += -std=c++14 -O2 -Isrc -Wno-ignored-attributes -Wno-delete-incomplete

# Microsoft XDK libraries to link against
XDK_LIBS := xapilib,d3d9,d3dx9,xgraphics,xnet,libcMT

include $(OXDK_DIR)/oxdk.mk

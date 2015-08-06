# ifdef check used for separate build Makefile - so we can just cd project_dir; make
ifndef $(SolutionDir)
SolutionDir := $(dir $(CURDIR))
ConfigurationName := Release
endif

include $(SolutionDir)common/gmakeprops/consolexe.mk

Includes += $(SolutionDir)cpcl

Libraries += libcpcl.a

SourceFiles := ./main.cpp ./task_pool.cpp ./connection.cpp ./http_parse.cpp ./image_cache.cpp ./jpeg_check_bgr.cpp ./jpeg_compressor_stuff.cpp ./jpeg_rendering_device.cpp ./run_server.cpp ./server.cpp
HeaderFiles := ./task_pool.h ./connection.h ./http_parse.hpp ./http_parser.h ./image_cache.h ./jpeg_compressor_stuff.h ./jpeg_rendering_device.h ./server.h

.PHONY: all
all: $(OutputFile)

include $(SolutionDir)common/gmakeprops/build_bin.mk

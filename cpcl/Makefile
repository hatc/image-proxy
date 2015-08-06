# ifdef check used for separate build Makefile - so we can just cd project_dir; make
ifndef $(SolutionDir)
SolutionDir := $(dir $(CURDIR))
ConfigurationName := Release
endif

include $(SolutionDir)common/gmakeprops/staticlib.mk

ifeq ($(ConfigurationName), Debug)
Preprocessor += CPCL_USE_BOOST_THREAD_SLEEP LIBXML_STATIC
else ifeq ($(ConfigurationName), Release)
Preprocessor += CPCL_USE_BOOST_THREAD_SLEEP LIBXML_STATIC
endif

Includes += $(SolutionDir)deps/libxml2-2.7.7-chromium/win32/include $(SolutionDir)deps/libxml2-2.7.7-chromium/src/include $(SolutionDir)deps/libxml2-2.7.7-chromium/win32

SourceFiles := ./dumbassert_posix.cpp ./dynamic_memory_stream.cpp ./error_handler.cpp ./file_iterator_posix.cpp ./file_stream_posix.cpp ./file_util_posix.cpp ./io_stream.cpp ./libxml_util.cpp ./memory_stream.cpp ./string_util.cpp ./timer_posix.cpp ./trace_posix.cpp
HeaderFiles := ./adapt_scl.hpp ./circular_iterator_adaptor.hpp ./com_ptr.hpp ./csv_reader.hpp ./dumbassert.h ./dynamic_memory_stream.h ./file_iterator.h ./file_stream.h ./file_util.h ./file_util.hpp ./formatidiv.hpp ./formatted_exception.hpp ./io_stream.h ./iterator_adapter.hpp ./libxml_util.h ./memory_limit_exceeded.hpp ./memory_stream.h ./scoped_buf.hpp ./split_iterator.hpp ./stdafx.h ./string_cast.hpp ./string_piece.hpp ./string_util.h ./string_util.hpp ./string_util_posix.hpp ./targetver.h ./timer.h ./trace.h ./trace_helpers.hpp

.PHONY: all
all: $(OutputFile)

include $(SolutionDir)common/gmakeprops/build_lib.mk

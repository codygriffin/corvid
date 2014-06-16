#-------------------------------------------------------------------------------
#
# corvid::Makefile
#
#  The MIT License (MIT)
#
#  Copyright (C) 2014 Cody Griffin (cody.m.griffin@gmail.com)
# 
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
#

#CXX     := /usr/local/bin/g++-4.9

all::
.PHONY: all

BIN_DIR := ./bin
SRC_DIR := ./src
INC_DIR := ./inc
TGT_DIR := ./tgt

#XXX Move to targets.mak
# No cross-compilation... yet
OS          := $(shell uname -s)
HOST        := $(OS)-$(shell uname -m)
TGT_DIR     := ./tgt/$(HOST)

include mak/helpers.mak
include mak/sources.mak
#include mak/boost.mak
include mak/application.mak
include mak/staticlib.mak
include mak/sharedlib.mak

CORVID_CXXFLAGS    := -std=gnu++0x -O0 -g
CORVID_CXXFLAGS    += -Iinc/
CORVID_LDFLAGS     := -pthread 
CORVID_SRCS        := src/Corvid.cpp #$(shell find src -name *.cpp) 

# Create all the targets for our application, and specify any compiler or linker dependencies
$(eval $(call APPLICATION,corvid))

PRATT_CXXFLAGS    := -std=gnu++0x -O0 -g
PRATT_CXXFLAGS    += -Iinc/
PRATT_LDFLAGS     := -pthread 
PRATT_SRCS        := src/Pratt.cpp 
$(eval $(call APPLICATION,pratt))

LEXER_CXXFLAGS    := -std=gnu++0x -O3 -g
LEXER_CXXFLAGS    += -Iinc/
LEXER_LDFLAGS     := -pthread 
LEXER_SRCS        := src/Lexer.cpp 
$(eval $(call APPLICATION,lexer))

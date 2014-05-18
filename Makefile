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
#  TODO Get a real build

CC      := g++ 

BIN_DIR := ./bin
SRC_DIR := ./src
LIB_DIR := ./lib
OBJ_DIR := ./obj
INC_DIR := ./inc
TST_DIR := ./tst
TGT_DIR := ./tgt
DEP_DIR := ./dep

clean:
	rm -f ./bin/*
	rm -f ./lib/*
	rm -f ./obj/*

all::
	# Running all tests...
	
.PHONY: all

$(BIN_DIR)/corvid: src/Corvid.cpp inc/Corvid.h inc/Bindings.h
	$(CC) -O0 -g -pg -o $@ -std=gnu++0x -pthread -L$(LIB_DIR) -lsqlite3 src/Corvid.cpp -I$(INC_DIR) -MMD -MT $@ -MF $(DEP_DIR)/$(notdir $(basename $<)).d 

corvid.gdb: $(BIN_DIR)/corvid
	# Executing corvid
	gdb $<

corvid.run: $(BIN_DIR)/corvid
	# Debugging corvid
	$<

corvid: $(BIN_DIR)/corvid

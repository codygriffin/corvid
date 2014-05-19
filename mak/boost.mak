#-------------------------------------------------------------------------------
#
# boost.mak
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

# Some boost definitions
BOOST       := boost_1_55_0.tar.gz
BOOST_BUILD := ./cots/boost_1_55_0
BOOST_LIBS  ?= date_time program_options exception log thread system filesystem serialization

BOOST_PATH  := $(TGT_DIR)/boost
BOOST_INC   := $(BOOST_PATH)/include
BOOST_LIB   := $(BOOST_PATH)/lib

# Create a list of the static libs that need to be linked
STATIC_BOOST_LIBS := $(foreach lib,$(BOOST_LIBS), $(BOOST_LIB)/libboost_$(lib).a)

$(BOOST_BUILD):
	# Extracting $(BOOST)
	cd cots; tar -xf $(BOOST)

BOOST_JOINED_LIBS := $(shell echo $(BOOST_LIBS) | sed 's/ /,/g')

$(BOOST_PATH): $(BOOST_BUILD)
	mkdir -p $(BOOST_PATH)
	cd $(BOOST_BUILD); \
	patch boost/atomic/detail/cas128strong.hpp < ../cas128strong.patch; \
	patch boost/atomic/detail/gcc-atomic.hpp < ../gcc-atomic.patch; \
	./bootstrap.sh --with-libraries=$(BOOST_JOINED_LIBS) --prefix=../../$(BOOST_PATH); \
	./b2 -j12 link=static threading=multi address-model=64 cxxflags=-std=c++11 install

$(BOOST_LIB) $(BOOST_INC): $(BOOST_PATH)
.INTERMEDIATE: $(BOOST_PATH)

boost: $(BOOST_LIB) $(BOOST_INC)

boost.clean:
	rm -rf $(BOOST_BUILD)
	rm -rf $(BOOST_PATH)

clean:: boost.clean

.PHONY: boost boost.clean

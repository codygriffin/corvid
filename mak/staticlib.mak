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

#-------------------------------------------------------------------------------
#
# STATICLIB($1 = appname, $2 = cxxdeps)
#
define STATICLIB

$(1)_SRCS     := $$($$(call UPCASE,$1)_SRCS)
$(1)_CFLAGS   := $$($$(call UPCASE,$1)_CFLAGS)
$(1)_CXXFLAGS := $$($$(call UPCASE,$1)_CXXFLAGS)

# Define our app-specific target directories
TGT_DIR       ?= $$(error 'TGT_DIR must be defined for $1')
$(1)_OBJ_DIR  := $$(TGT_DIR)/$1/obj
$(1)_DEP_DIR  := $$(TGT_DIR)/$1/dep
$(1)_LIB_DIR  := $$(TGT_DIR)/$1/lib

# Build our obj and dep paths from our sources 
# TODO add C
$(1)_OBJS    := $$(addprefix $$($(1)_OBJ_DIR)/,$$(notdir $$($(1)_SRCS:.cpp=.o)))
$(1)_DEPS    := $$(addprefix $$($(1)_DEP_DIR)/,$$(notdir $$($(1)_SRCS:.cpp=.d)))

# TODO this should move out of here
$(1)_OBJS    += $$(addprefix $$($(1)_OBJ_DIR)/,$$(notdir $$(CUDA_SRCS:.cu=.o)))

# Add the directories for our sources to our VPATH 
VPATH        += $$(sort $$(dir $$($(1)_SRCS)))

$$(eval $$(call COMPILE_CPP,$1,$2))
$$(eval $$(call COMPILE_C,$1,$2))
$$(eval $$(call COMPILE_KERNEL,$1,$2))

$$($(1)_LIB_DIR)/$1.a: $$($(1)_OBJS) $3
	# Building static library $$@
	@mkdir -p $$($(1)_LIB_DIR)
	$$(AR) cr $$@ $$? 

-include $$($(1)_DEPS)

$1: $$($(1)_LIB_DIR)/$1.a

$1.clean:
	rm -rf $$(TGT_DIR)/$1

clean:: $1.clean

all:: $1

.PHONY: $1 $1.clean

endef

#-------------------------------------------------------------------------------
#
# application.mak
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
# COMPILE_CPP($1 = appname, $2 = cxxdeps)
#
define COMPILE_CPP
$$($(1)_OBJ_DIR)/%.o : %.cpp $2
	# Compiling $$<
	@mkdir -p $$($(1)_OBJ_DIR)
	@mkdir -p $$($(1)_DEP_DIR)
	$$(CXX) $$($(1)_CXXFLAGS) -c $$< -o $$@ \
  -MMD -MT '$$@' -MF $$($(1)_DEP_DIR)/$$(notdir $$(basename $$<)).d 
endef

#-------------------------------------------------------------------------------
#
# COMPILE_C($1 = appname, $2 = cxxdeps)
#
define COMPILE_C
$$($(1)_OBJ_DIR)/%.o : %.c $2
	# Compiling $$<
	@mkdir -p $$($(1)_OBJ_DIR)
	@mkdir -p $$($(1)_DEP_DIR)
	$$(CC) $$($(1)_CFLAGS) -c $$< -o $$@ \
  -MMD -MT '$$@' -MF $$($(1)_DEP_DIR)/$$(notdir $$(basename $$<)).d 
endef

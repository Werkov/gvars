#!/bin/bash

#
# See: http://belion.tumblr.com/post/36151777927/ptam-compilation-notes
#

DIR="`cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd`"
CXXFLAGS="-Wno-unused-variable"
./configure --disable-widgets --prefix=$DIR/../../Build/gvars

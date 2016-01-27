#########################################################################
#
#  Copyright 2009-2011 Intel Corporation.  All Rights Reserved.
#
#  The source code contained or described herein and all documents related
#  to the source code ("Material") are owned by Intel Corporation or its
#  suppliers or licensors.  Title to the Material remains with Intel
#  Corporation or its suppliers and licensors.  The Material is protected
#  by worldwide copyright laws and treaty provisions.  No part of the
#  Material may be used, copied, reproduced, modified, published, uploaded,
#  posted, transmitted, distributed, or disclosed in any way without
#  Intel's prior express written permission.
#
#  No license under any patent, copyright, trade secret or other
#  intellectual property right is granted to or conferred upon you by
#  disclosure or delivery of the Materials, either expressly, by
#  implication, inducement, estoppel or otherwise.  Any license under such
#  intellectual property rights must be express and approved by Intel in
#  writing.
###########################################################################

TOP = ..
include $(TOP)/mk/win-common.mk

#
# Convert README and CHANGES.txt to html
#

# Convert any (R)'s to &reg;
SED_EXPR := -e 's/(R)/<sup>\&reg;<\/sup>/g'

# Convert any (TM)'s to &trade;
SED_EXPR += -e 's/(TM)/<sup>\&trade;<\/sup>/g'

# Convert any URLs to links
SED_EXPR += -e 's/http:\/\/[^ ]*[^.]/<a href=\"&\">&<\/a>/g'

FILES := $(PROD-OUT)/README.html $(PROD-OUT)/CHANGES.html
.PHONY: all
all: $(FILES)

$(PROD-OUT)/README.html : README
	# Add the prefix
	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">' > $@
	echo '<html xmlns="http://www.w3.org/1999/xhtml" >' >> $@
	echo '<head>' >> $@
	echo '    <title>Intel&reg; Cilk&trade; Plus runtime library README</title>' >> $@
	echo '</head>' >> $@
	echo '<body>' >> $@
	echo '<pre>' >> $@
	# Convert any (R)'s to &reg;, (TM)'s to &trade; and URLs to links
	sed $(SED_EXPR) < $< >> $@
	# Add the suffix
	echo '</pre>' >> $@
	echo '</body>' >> $@
	echo '</html>' >> $@

$(PROD-OUT)/CHANGES.html: ../runtime/CHANGES.txt
	# Add the prefix
	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">' > $@
	echo '<html xmlns="http://www.w3.org/1999/xhtml" >' >> $@
	echo '<head>' >> $@
	echo '    <title>Intel&reg; Cilk&trade; Plus runtime library Change log</title>' >> $@
	echo '</head>' >> $@
	echo '<body>' >> $@
	echo '<pre>' >> $@
	# Convert any (R)'s to &reg;, (TM)'s to &trade; and URLs to links
	sed $(SED_EXPR) < $< >> $@
	# Add the suffix
	echo '</pre>' >> $@
	echo '</body>' >> $@
	echo '</html>' >> $@

.PHONY: clean
clean:
	rm -f $(FILES)

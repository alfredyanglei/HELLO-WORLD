
#-----------------------------------------------------------------------

APP       := HQExtend
TARGET    := ConditionServer
#DFLAGS    := all/O2/g
DFLAGS    := g
CONFIG    := 
STRIP_FLAG:= N
J2CPP_FLAG:= 

INCLUDE_DIR  = $(shell mysql_config --include)
INCLUDE_LIBS = $(shell mysql_config --libs)
INCLUDE     += $(INCLUDE_DIR)
LIB         += $(INCLUDE_LIBS)

ifdef  TSERVER
TARGET    := ConditionTServer
else
TARGET    := ConditionServer
endif
RELEASE_TARGET = ConditionServer

#-----------------------------------------------------------------------

include /home/tafjce/HQSys/FileDataServer/FileDataServer.mk
include /home/tafjce/HQSys/BasicHqServer/BasicHqServer.mk

include /home/tafjce/HQSys/LIBS/hqtool/hqtool.mk
include /home/tafjce/HQSys/LIBS/common/common.mk
include /home/tafjce/HQSys/LIBS/hqutils/hqutils.mk
include /usr/local/taf/cpp/makefile/makefile.taf.new

#-----------------------------------------------------------------------

uploadT:
		make upload TSERVER=1

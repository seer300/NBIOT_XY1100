####################################################################################################
# directory
####################################################################################################
LFS_SRC_DIR := $(APPLIB_SRC_DIR)/fs


####################################################################################################
# source file
####################################################################################################
SRCS_LFS_C_RAM = \

SRCS_LFS_C_FLASH = \
	$(wildcard $(LFS_SRC_DIR)/littlefs/*.c) \
	$(wildcard $(LFS_SRC_DIR)/xy_fs/*.c)
#----------------------------------------------------------------------- \
# if need add more file, move the bottom row to the top \
#----------------------------------------------------------------------- \


SRCS_LFS_EXCLUDE_LIB_RAM = \
	
#----------------------------------------------------------------------- \
# if need add more file, move the bottom row to the top \
#----------------------------------------------------------------------- \


SRCS_LFS_EXCLUDE_LIB_FLASH = \
#----------------------------------------------------------------------- \
# if need add more file, move the bottom row to the top \
#----------------------------------------------------------------------- \


SRCS_LFS_EXCLUDE_PRJ = \
#----------------------------------------------------------------------- \
# if need add more file, move the bottom row to the top \
#----------------------------------------------------------------------- \


####################################################################################################
# source
####################################################################################################
TMP_LFS_FILE := $(filter-out $(SRCS_LFS_C_RAM), $(SRCS_LFS_EXCLUDE_LIB_FLASH))
TMP_LFS_FILE := $(filter-out $(SRCS_LFS_C_FLASH), $(TMP_LFS_FILE))
TMP_LFS_FILE := $(filter-out $(SRCS_LFS_EXCLUDE_LIB_RAM), $(TMP_LFS_FILE))
TMP_LFS_FILE := $(filter-out $(SRCS_LFS_EXCLUDE_PRJ), $(TMP_LFS_FILE))

SRCS_LFS_RAM   += $(SRCS_LFS_C_RAM)
SRCS_LFS_FLASH += $(SRCS_LFS_C_FLASH)

LFS_EXCLUDE_LIB_RAM   += $(SRCS_LFS_EXCLUDE_LIB_RAM)
LFS_EXCLUDE_LIB_FLASH += $(TMP_LFS_FILE)


####################################################################################################
# include path
####################################################################################################
DEPS_PATH_LFS += $(LFS_SRC_DIR)/littlefs
DEPS_PATH_LFS += $(LFS_SRC_DIR)/xy_fs
####################################################################################################
# add to sysapp.mk
####################################################################################################
SRCS_APPLIB_RAM   += $(SRCS_LFS_RAM)
SRCS_APPLIB_FLASH += $(SRCS_LFS_FLASH)

APPLIB_EXCLUDE_LIB_RAM   += $(LFS_EXCLUDE_LIB_RAM)
APPLIB_EXCLUDE_LIB_FLASH += $(LFS_EXCLUDE_LIB_FLASH)

DEPS_PATH_APPLIB += $(DEPS_PATH_LFS)
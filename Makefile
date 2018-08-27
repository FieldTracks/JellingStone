#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := jellingstone

COMPONENT_ADD_INCLUDEDIRS := components/include

include $(IDF_PATH)/make/project.mk

stone: flash
	python $(IDF_PATH)/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py nvs_data.csv build/nvs.bin
	python $(IDF_PATH)/components/esptool_py/esptool/esptool.py write_flash 0x9000 build/nvs.bin

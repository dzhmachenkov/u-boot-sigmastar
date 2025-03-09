CONFIG_SYS_CONFIG_NAME := $(subst ",,$(CONFIG_SYS_CONFIG_NAME))

ccflags-y += -I$(srctree)/$(src)/$(CONFIG_SYS_CONFIG_NAME)
ccflags-y += -I$(srctree)/$(src)

obj-y += font.o
obj-y += draw2d.o
obj-y += ApkFactory.o
obj-y += I2CBusSW.o
obj-y += ApkSettingsUboot.o
obj-y += ApkUbootMtdParts.o
obj-y += ApkFlashProtect.o
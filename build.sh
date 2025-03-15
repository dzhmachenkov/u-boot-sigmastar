#!/bin/sh

PROJ_PATH=$PWD/../project

declare -x ARCH="arm"

case "$1" in
"")
	if [ "$FLASH_TYPE" == "nor" ];then
		echo "uboot >> nor"
		make infinity6b0_defconfig
		make -j4
		#cp ./u-boot.xz.img.bin $PROJ_PATH/board/i6b0/boot/nor/uboot/u-boot.xz.img.cardv.bin
		cp ./u-boot.xz.img.bin $PROJ_PATH/board/i6b0/boot/nor/uboot/u-boot.xz.img.bin
		mv u-boot.xz.img.bin u-boot.xz.img.cardv.bin
		cp u-boot.xz.img.cardv.bin UBOOT
	else
		echo "uboot >> nand"
		make infinity6b0_spinand_defconfig
		make -j4
		cp ./u-boot_spinand.xz.img.bin $PROJ_PATH/board/i6b0/boot/spinand/uboot/u-boot_spinand.xz.img.bin
		mv u-boot_spinand.xz.img.bin UBOOT
	fi
    ;;
setup)
    if [ "$FLASH_TYPE" == "nor" ];then
		echo "uboot >> nor"
		make infinity6b0_defconfig
	else
		echo "uboot >> nand"
		make infinity6b0_spinand_defconfig
	fi
	;;
clean)
    make clean
    ;;
distclean)
    make distclean
    ;;
esac

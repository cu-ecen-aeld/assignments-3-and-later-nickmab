#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
TOOLCHAIN=aarch64-none-linux-gnu
CROSS_COMPILE=${TOOLCHAIN}-
TOOLCHAIN_DIR=$(readlink -f $(which "${CROSS_COMPILE}gcc") | sed -E "s@(.+)/bin/${CROSS_COMPILE}gcc@\1/${TOOLCHAIN}@")
FINDER_APP_DIR=$HOME/repo/coursera-buildroot/finder-app

if [ ! -d $TOOLCHAIN_DIR ]
then
    echo "Unable to find TOOLCHAIN_DIR: ${TOOLCHAIN_DIR}"
    exit 1
else
    echo "Using toolchain at: ${TOOLCHAIN_DIR}"
fi

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make mrproper
    ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make defconfig
    ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make -j3
    ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make modules
    ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir rootfs
cd rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    make distclean
    make defconfig
else
    cd busybox
fi

ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make
ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE make CONFIG_PREFIX="${OUTDIR}/rootfs" install

echo "Library dependencies:"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "program interpreter"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "Shared library"

# TODO: Make this more automated!
cp "${TOOLCHAIN_DIR}/libc/lib/ld-linux-aarch64.so.1" "${OUTDIR}/rootfs/lib/"
cp "${TOOLCHAIN_DIR}/libc/lib64/libc.so.6" "${OUTDIR}/rootfs/lib64/"
cp "${TOOLCHAIN_DIR}/libc/lib64/libm.so.6" "${OUTDIR}/rootfs/lib64/"
cp "${TOOLCHAIN_DIR}/libc/lib64/libresolv.so.2" "${OUTDIR}/rootfs/lib64/"

sudo mknod -m 666 "${OUTDIR}/rootfs/dev/null" c 1 3
sudo mknod -m 666 "${OUTDIR}/rootfs/dev/console" c 5 1

cd $FINDER_APP_DIR
CROSS_COMPILE=$CROSS_COMPILE make all
cp ./writer "${OUTDIR}/rootfs/home"
# trailing slash is important on the /conf/ dir as /conf is a symlink to another dir.
cp -R ./conf/ "${OUTDIR}/rootfs/home/conf"
cp ./autorun-qemu.sh "${OUTDIR}/rootfs/home"
cp ./finder.sh "${OUTDIR}/rootfs/home"
cp ./finder-test.sh "${OUTDIR}/rootfs/home"

sudo chown root:root "${OUTDIR}/rootfs/bin/busybox"
sudo chmod u+s "${OUTDIR}/rootfs/bin/busybox"

cd "${OUTDIR}/rootfs/"
find . | cpio -H newc -ov --owner root:root > "${OUTDIR}/initramfs.cpio"
cd $OUTDIR
gzip -f initramfs.cpio

#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
COMPILER=/home/zaid/work/tools/gcc/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu

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
    cd linux-stable
    git apply $FINDER_APP_DIR/../dtc-multiple-definition.patch
    cd -
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE -j4 all dtbs
fi

echo "Adding the Image in outdir"
KERNEL=${OUTDIR}/linux-stable
cp $KERNEL/arch/arm64/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
ROOTFS=${OUTDIR}/rootfs
mkdir -p $ROOTFS
cd $ROOTFS
mkdir -p bin sbin lib lib64 var etc tmp dev proc include usr home sys
mkdir usr/bin usr/sbin usr/lib
mkdir -p var/log

echo "Creating Busybox"
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make clean
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX=$ROOTFS install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cp $COMPILER/libc/lib/ld-linux-aarch64.so.1 $ROOTFS/lib/
cp $COMPILER/libc/lib64/libm.so.6 $ROOTFS/lib64/
cp $COMPILER/libc/lib64/libc.so.6 $ROOTFS/lib64/
cp $COMPILER/libc/lib64/libresolv.so.2 $ROOTFS/lib64/

# TODO: Make device nodes
echo "Creating dev nodes"
cd $ROOTFS
sudo mknod -m 666 dev/null c 1 3 
sudo mknod -m 666 dev/console c 5 1 

# TODO: Clean and build the writer utility
echo "Creating writer"
cd $FINDER_APP_DIR
make clean 
make CROSS_COMPILE=$CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer $ROOTFS/home/
cp finder.sh $ROOTFS/home/
cp finder-test.sh $ROOTFS/home/
cp autorun-qemu.sh $ROOTFS/home/

mkdir -p $ROOTFS/home/conf
cp conf/username.txt $ROOTFS/home/conf/
cp conf/assignment.txt $ROOTFS/home/conf/

# TODO: Chown the root directory
echo "chowning rootfs"
sudo chown -R root $ROOTFS

#tree $ROOTFS

# TODO: Create initramfs.cpio.gz
echo "Creating initramfs"
cd  $ROOTFS
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
cd -
echo "Done!"


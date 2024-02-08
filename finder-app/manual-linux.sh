#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -x
set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=/home/embeddedmaster/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

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

    # TODO: Add your kernel build steps here
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE clean
    sed -i 's/YYLTYPE yylloc;/extern &/' ./scripts/dtc/dtc-lexer.l
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}/"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
sudo mkdir rootfs
sudo mkdir -p rootfs/bin rootfs/dev rootfs/etc rootfs/home rootfs/lib rootfs/lib64 rootfs/proc rootfs/sbin rootfs/sys rootfs/tmp rootfs/usr rootfs/var
sudo mkdir -p rootfs/usr/bin rootfs/usr/lib rootfs/usr/sbin
sudo mkdir -p rootfs/var/log
CURRENT_USER=$(whoami)
CURRENT_GROUP=$(id -gn)
sudo chown -R "${CURRENT_USER}:${CURRENT_GROUP}" "${OUTDIR}/rootfs"


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
# git clone https://github.com/mirror/busybox.git
git clone https://github.com/mirror/busybox.git


    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

# TODO: Make and install busybox
sudo make distclean
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
sed -i 's/CONFIG_EXTRA_CFLAGS=""/CONFIG_EXTRA_CFLAGS="-static"/g' .config   
sudo make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} install

# sudo chmod +s ${OUTDIR}/rootfs/bin/busybox commented by me


cd ${OUTDIR}/rootfs 

echo "Library dependencies"
# ${CROSS_COMPILE}readelf -a /bin | grep "program interpreter" 
# ${CROSS_COMPILE}readelf  -a /bin | grep "Shared library"
echo "After checking library dependencies"

# TODO: Add library dependencies to rootfs

# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
sudo cp -RH * ${OUTDIR}/rootfs/home
cd ${FINDER_APP_DIR}/conf
sudo cp -RH * ${OUTDIR}/rootfs/home

cd ${OUTDIR}/rootfs/home
sed -i 's|cat ../conf/assignment.txt|cat conf/assignment.txt|' finder-test.sh
sudo chmod +x finder-test.sh
sudo chmod +x writer.sh
# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f $OUTDIR/initramfs.cpio
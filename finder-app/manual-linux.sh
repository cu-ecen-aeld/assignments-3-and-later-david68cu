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
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "**** Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "**** Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "**** CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "**** Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # When we did above git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git --depth 1 --single-branch --branch v5.15.163
    # we end up with a folder linux-stable/ of 212 MB and tons of code that is the linux kernel. It is bascially a large C project
    # The Makefile in the root of linux-stable/ folder is 2038 lines large . Now we need to build the kernel as we were taught in lesson "Building the Linux Kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # Load a default configuration
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # Compile the kernel. 
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all -j$(nproc) 

    # Build the modules : Installs the compiled kernel modules (.ko files) into the root filesystem at ${OUTDIR}/rootfs/lib/modules/$(uname -r)/.
    # Ensures modules are included in initramfs or available at runtime.
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${OUTDIR}/rootfs modules_install
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
fi

echo "**** Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "**** Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "**** Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories as per lecture "Buidling the root file system"
# LAter on the script will need to use some of this idrectories for example ${OUTDIR}/rootfs/lib64.
mkdir -p ${OUTDIR}/rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,var}
cd ${OUTDIR}/rootfs
mkdir -p usr/bin usr/sbin usr/lib var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox as per lecture "Buidling the root file system" 
    # BusyBox is a lightweight utility suite that provides essential Linux commands in a single binary
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox. See what are the needed librarirs from the Metadata of the ELF
echo "**** Make an install  BusyBox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "**** Analyzing  bin/busybox ELF (Executable and Linkable Format) files for aarch64-none-linux-gnu- architecture. Getting Metadata for busybox"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs. Busybox depends on certain libraries, those libraries 
# must be present within the rootfs for Busybox to execute correctly
# I will do it in 3 Steps
echo "*** Add busybox library dependencies to rootfs"
# 1. Find the compiler path to determine the toolchain location
compiler_path=$(which aarch64-none-linux-gnu-gcc)  
toolchain_path=$(dirname $(dirname $compiler_path)) 

echo "Toolchain path: $toolchain_path" # Helpful for debugging

# 2. Find and copy the interpreter (dynamic linker)
# The interpreter /opt/arm-cross-compiler/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 , 
# must be copied to ${OUTDIR}/rootfs/lib/
# If the interpreter is missing, the kernel cannot execute the init process, you will get a Kernel Panic
# When a dynamically linked executable (such as BusyBox) is run, the kernel uses the interpreter to load the necessary 
# shared libraries into memory and resolve symbols before transferring control to the executable.
echo "Finding and copying the interpreter..."
interpreter=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | awk '{print $NF}' | tr -d '[]')
interpreter_path=$(find $toolchain_path -name $(basename $interpreter))


# Resolve the symlink to its target
interpreter_target=$(readlink -f "$interpreter_path")  # -f follows all levels of symlinks

# This could be a simpler solution but could not work cp -L "$interpreter_path" ${OUTDIR}/rootfs/lib/
# The objective is to copy /opt/arm-cross-compiler/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 to ${OUTDIR}/rootfs/lib/
# and also the versioned dynamic linker/loader ld-2.33.so
if [[ -f "$interpreter_target" ]]; then
    cp "$interpreter_target" ${OUTDIR}/rootfs/lib/  # Copy the TARGET
    cp "$interpreter_path" ${OUTDIR}/rootfs/lib/  # Copy the LINK
    echo "Interpreter target ld-linux-aarch64.so.1 and ld-2.33.so COPIED TO  ${OUTDIR}/rootfs/lib/."
else
    echo " ${interpreter_target} or interpretar path "$interpreter_path not found " # Copy the LINK"
    echo ".. ERROR copying interpreter ld-linux-aarch64.so.1 or ld-2.33.so ${OUTDIR}/rootfs/lib/ or versioned dynamic linker/loader ld-2.33.so ."
    exit 1 
fi


# 3. Find and copy the shared libraries
echo "Finding and copying shared libraries..."

# We have a lib64 directory based on architecture.  
lib_dir="lib64" # Default. Change to "lib" if needed for your architecture

for lib in $(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" | awk '{print $NF}' | tr -d '[]')
do
    echo "Library: $lib" # Helpful for debugging
    lib_path=$(find $toolchain_path -name "$lib")  # Quote $lib for filenames with spaces

    if [[ -f "$lib_path" ]]; then # Check if library was found
        cp "$lib_path" ${OUTDIR}/rootfs/"$lib_dir"/ # Copy to rootfs/lib or lib64
        echo "Library $lib copied successfully."
    else
        echo "Error: Library $lib not found!"
        exit 1 # Or handle the error differently
    fi
done

echo "Library dependencies added to rootfs."







# TODO: Make device nodes
echo "*** Making device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
# Lets use sudo /dev/tty 5 0 , some kernels could need the terminal
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty c 5 0
# And also sudo /dev/console 5 1 , some kernels could need th console
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1



# TODO: Clean and build the writer utility
echo "*** Clean and build the writer utility"
make -C ${FINDER_APP_DIR} clean
make -C ${FINDER_APP_DIR} CROSS_COMPILE=${CROSS_COMPILE}
# cd ${FINDER_APP_DIR}
# make clean
# make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
echo "*** copy the finder related scripts and executables to the /home directory"
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home
# Copy the conf/ folder
mkdir -p "${OUTDIR}"/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf


# TODO: Chown the root directory
echo "*** chown the root directory"
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz

# The kernel could need an init file
echo "*** Creating init script for initramfs"

sudo tee ${OUTDIR}/rootfs/init > /dev/null << EOF
#!/bin/sh
echo "Hello from init!"
mount -t proc none /proc
mount -t sysfs none /sys
exec /bin/busybox sh
EOF

sudo chmod +x ${OUTDIR}/rootfs/init


# The initramfs (initial RAM filesystem) acts as a temporary root filesystem that is loaded into RAM very early in the boot process.
# Think of it as a mini-filesystem that provides the essential tools and drivers needed to mount the real root filesystem.
# The initramfs is usually compressed (using gzip) to reduce its size.  This makes it faster to load into memory, which is important for a quick boot time. 
# The kernel knows how to decompress the initramfs on the fly.


echo "*** Create initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
echo "initramfs.cpio.gz created successfully in ${OUTDIR}"

# We should be able to veryfy the created zip file. First create  a temprary directory
# mkdir -p /tmp/aeld/check_init_ram_image
# cd /tmp/aeld/check_init_ram_image
# gunzip -c /tmp/aeld/initramfs.cpio.gz | cpio -idmv
# Observe teh structure of teh file system
# ls -l /tmp/aeld/check_init_ram_image/init

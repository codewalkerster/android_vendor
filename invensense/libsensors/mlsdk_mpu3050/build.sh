MAKE_CMD="make \
    VERBOSE=1 \  
    TARGET=android \
    DEVICE=MPU3050 \
    CROSS=/home/odroid/project/pegasus/android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- \ 
    ANDROID_ROOT=/home/odroid/project/pegasus/bsp02_android \
    KERNEL_ROOT=/home/odroid/project/pegasus/bsp02_kernel \
    PRODUCT=odroida\
"
eval $MAKE_CMD -f Android-static.mk clean
eval $MAKE_CMD -f Android-shared.mk clean
eval $MAKE_CMD -f Android-static.mk 
eval $MAKE_CMD -f Android-shared.mk
# eval $MAKE_CMD -f Android-shared.mk clean

cp -rf ./mllite/mpl/android/libmllite.so ./libmllite3050.so
cp -rf ./mldmp/mpl/android/libmplmpu.so ./libmplmpu3050.so
cp -rf ./platform/linux/libmlplatform.so ./libmlplatform3050.so

MAKE_CMD="make \
    VERBOSE=1 \  
    TARGET=android \
    DEVICE=MPU6050B1 \
    CROSS=/home/odroid/project/jb/android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- \
    ANDROID_ROOT=/home/odroid/project/jb/android \
    KERNEL_ROOT=/home/odroid/project/kernel \
    PRODUCT=odroidq\
"
eval $MAKE_CMD -f Android-static.mk clean
eval $MAKE_CMD -f Android-shared.mk clean
eval $MAKE_CMD -f Android-static.mk 
eval $MAKE_CMD -f Android-shared.mk
# eval $MAKE_CMD -f Android-shared.mk clean

cp -rf ./mllite/mpl/android/libmllite.so .
cp -rf ./mldmp/mpl/android/libmplmpu.so .
cp -rf ./platform/linux/libmlplatform.so .

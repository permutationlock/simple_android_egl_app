# required environment:
#     ANDROID_SDK_BUILD_TOOLS: path to android sdk build_tools
#     ANDROID_NDK_TOOLCHAIN: path to android ndk toolchain prebuilt sysroot
#     ANDROID_VERSION: the target android version number (must be >=22)
#     ANDROID_JAR: a path to the android-XX.jar file (see pull_android_jar.sh)
#     APP_NAME: the name of your app
#     ORG_NAME: the name of your organization
#     KEYSTORE_FILE: path to keystore file
#     STORE_PASS: keystore password
#     KEY_PASS: key password
#     CFLAGS: common flags to be passed to C compiler
#     LDFLAGS: common flags to be passed to the linker

# clean old build artifacts
./clean.sh

# initualize build directory from template
cp -r ./template ./build_android
envsubst '$$ANDROID_VERSION $$APP_NAME $$ORG_NAME' < ./template/AndroidManifest.xml > ./build_android/AndroidManifest.xml

# build so for arm64
mkdir -p ./build_android/apk/lib/arm64-v8a
$ANDROID_NDK_TOOLCHAIN/bin/aarch64-linux-android$ANDROID_VERSION-clang $CFLAGS $LDFLAGS -shared -fPIC -lm -ldl -landroid -llog -m64 -I./include/ -o ./build_android/apk/lib/arm64-v8a/lib$APP_NAME.so ./src/main.c ./src/android_native_app_glue.c

# build so for arm32
mkdir -p ./build_android/apk/lib/armeabi-v7a
$ANDROID_NDK_TOOLCHAIN/bin/armv7a-linux-androideabi$ANDROID_VERSION-clang $CFLAGS $LDFLAGS -shared -fPIC -lm -ldl -landroid -llog -mfloat-abi=softfp -m32 -I./include/ -o ./build_android/apk/lib/armeabi-v7a/lib$APP_NAME.so ./src/main.c ./src/android_native_app_glue.c

# build so for x86
mkdir -p ./build_android/apk/lib/x86
$ANDROID_NDK_TOOLCHAIN/bin/i686-linux-android$ANDROID_VERSION-clang $CFLAGS $LDFLAGS -shared -fPIC -lm -ldl -landroid -llog -march=i686 -mssse3 -mfpmath=sse -m32 -I./include/ -o ./build_android/apk/lib/x86/lib$APP_NAME.so ./src/main.c ./src/android_native_app_glue.c

# build for x86_64
mkdir -p ./build_android/apk/lib/x86_64
$ANDROID_NDK_TOOLCHAIN/bin/x86_64-linux-android$ANDROID_VERSION-clang $CFLAGS $LDFLAGS -shared -fPIC -lm -ldl -landroid -llog -m64 -msse4.2 -mpopcnt -I./include/ -o ./build_android/apk/lib/x86_64/lib$APP_NAME.so ./src/main.c ./src/android_native_app_glue.c

# build temporary apk and unzip back to directory
$ANDROID_SDK_BUILD_TOOLS/aapt package -f -F ./build_android/temp.apk -I $ANDROID_JAR -M ./build_android/AndroidManifest.xml -S ./build_android/apk/res -v --target-sdk-version $ANDROID_VERSION
unzip -o ./build_android/temp.apk -d ./build_android/apk

# ALTERNATIVE: since aapt is deprecated, we could also use aapt2:
#   # build temporary apk and unzip back to directory
#   mkdir ./build_android/compiled
#   $ANDROID_SDK_BUILD_TOOLS/aapt2 compile ./build_android/apk/res/mipmap/icon.png -o ./build_android/compiled/
#   $ANDROID_SDK_BUILD_TOOLS/aapt2 link -o ./build_android/temp.apk -I $ANDROID_JAR --manifest ./build_android/AndroidManifest.xml ./build_android/compiled/mipmap_icon.png.flat -v --target-sdk-version $ANDROID_VERSION
#   unzip -o ./build_android/temp.apk -d ./build_android/apk

# zip compress second temporary apk
cd ./build_android/apk
zip -D4r ../temp2.apk .
zip -D0r ../temp2.apk ./resources.arsc ./AndroidManifest.xml
cd ../..

# align and sign final apk
$ANDROID_SDK_BUILD_TOOLS/zipalign -v 4 ./build_android/temp2.apk ./build_android/$APP_NAME.apk
$ANDROID_SDK_BUILD_TOOLS/apksigner sign --key-pass pass:$KEY_PASS --ks-pass pass:$STORE_PASS --ks $KEYSTORE_FILE ./build_android/$APP_NAME.apk

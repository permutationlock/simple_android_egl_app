# Simple Native Android EGL App

A minimal example of building a C Android NDK app from the command line without
writing java code. I also wrote an [accompanying article][9].

## Motivation

I currently use a musl libc based distro (Chimera Linux) and Google does not
distribute Android Studio for such a system. Luckily, a good samaritan has gone through
the work of compiling the [Android SDK tools][3] and [Android NDK][4] into
static binaries that will work on any Linux system.

The [rawdraw Android][1] project provides a fantastic example of how to build a native
Android applicaiton from the command line, but it is still somewhat
complicated. After getting my [GLFW applications][2] to compile for
Android, I distilled the process down to this tiny OpenGL ES 2.0 example.

## Dependencies

You'll need to install [OpenJDK][5], and acquire
a copy of the [Android SDK][6] command line tools (`aapt` or `aapt2`, `zipalign`, `apksigner`),
and the [Android NDK][7] `clang` toolchain. See [my article][9] for a walkthrough
setting up a build environment.

You will also need a copy of
`android.jar` for the target android SDK version. A copy can be pulled from the
[Sable collection][8] by running `ANDROID_VERSION=XX ./pull_android_jar.sh`.

Finally, you will need a keystore file to sign your application. A dummy
debug keystore can be generated using `keytool` from OpenJDK:

```bash
keytool -genkey -v -keystore mykey.keystore -alias mykey -keyalg RSA -keysize 2048 \
    -validity 10000 -storepass mypassword -keypass mypassword \
    -dname "CN=example.com, OU=ID, O=Example, L=Callar, S=Morvern, C=GB"
```

## Building

You will need to export the following environment variables in order to run `./build.sh`.

```bash
#     ANDROID_VERSION: the target android version number (must be >=22)
#     ANDROID_JAR: a path to the android.jar file (see pull_android_jar.sh)
#     ANDROID_AAPT: path to android sdk aapt
#     ANDROID_ZIPALIGN: path to android sdk zipalign
#     ANDROID_APKSIGNER: path to android sdk apksizinger
#     ANDROID_CLANG: path to android ndk clang
#     APP_NAME: the name of your app
#     ORG_NAME: the name of your organization
#     KEYSTORE_FILE: path to keystore file
#     STORE_PASS: keystore password
#     KEY_PASS: key password
#     CFLAGS: common flags to be passed to C compiler
#     LDFLAGS: common flags to be passed to the linker
```

An example build command:

```bash
ANDROID_VERSION=35 ./pull_android_jar.sh
ANDROID_VERSION=35 \
    ANDROID_JAR=./android-35.jar \
    ANDROID_AAPT=/home/user/android_sdk/build-tools/35.0.1/aapt \
    ANDROID_ZIPALIGN=/home/user/android_sdk/build-tools/35.0.1/zipalign \
    ANDROID_APKSIGNER=/home/user/android_sdk/build-tools/35.0.1/apksigner \
    ANDROID_CLANG=/home/user/android_sdk/ndk/29.0.13599879/toolchains/llvm/prebuilt/linux-x86_64/bin/clang \
    APP_NAME=seglapp \
    ORG_NAME=avensegl \
    KEYSTORE_FILE=./mykey.keystore \
    STORE_PASS=mypassword \
    KEY_PASS=mypassword \
    CFLAGS="-Wall -Wextra -Wno-unused-parameter -std=c11 -O3 -g0 -s" \
    ./build.sh
```

## Installing and testing

You will need to enable USB Debugging on the test device (or use an emulator) and then
install the APK with `adb`:

```bash
adb install ./build_android/seglapp.apk # replace seglapp.apk with your app name
```

To view debug logs you can run:

```bash
adb shell logcat SEGL:I *:S
```

To uninstall the app you can run:

```bash
adb uninstall org.avensegl.seglapp # replace avensegl and seglapp with your org and app names
```

[1]: https://github.com/cnlohr/rawdrawandroid
[2]: https://github.com/permutationlock/libavengraph
[3]: https://github.com/HomuHomu833/android-sdk-custom
[4]: https://github.com/HomuHomu833/android-ndk-custom
[5]: https://openjdk.org/index.html
[6]: https://developer.android.com/studio
[7]: https://developer.android.com/ndk/downloads
[8]: https://github.com/Sable/android-platforms
[9]: https://musing.permutationlock.com/android_egl/

# Simple Native Android EGL App

A minimal example of building a C Android NDK app from the command line without
java code (beyond the required `androidXX.jar`).

## Motivation

I am currently using a musl libc based distro (Chimera Linux) and google does not
distribute Android Studio for such a system. Some good samaritan has gone through
the work of compiling the [Android SDK build tools][3] and [Android NDK][4] for musl based
Linux systems.

The [rawdrawandroid][1] project provides a fantastic example of how to build a native
Android applicaiton from the command line, but it is still somewhat
complicated. Having gone through the work of [getting my GLFW applications to compile for
Android][2], I thought I would write out a minimal example that uses `dlopen` to load
`libEGL.so`, creates an OpenGL ES 2.0 context, and draws colors to the screen. The example
also shows how to properly handle suspend and resume as Android applications must relinquish
EGL contexts while suspended[^1].

## Dependencies

You'll also need an OpenJDK install,
a copy of the Android SDK build tools (`aapt`, `zipalign`, `apksigner`),
and a copy of the Android NDK toolchains.

You will also need a copy of
`androidXX.jar` where `XX` is the target android version. A copy can be pulled from a friendly git
repo by running `ANDROID_VERSION=XX ./pull_android_jar.sh`.

Finally, you will need a keystore file to sign your application. E.g. using `keytool`:
```
keytool -genkey -v -keystore mykey.keystore -alias mykey -keyalg RSA -keysize 2048 \
    -validity 10000 -storepass mypassword -keypass mypassword \
    -dname "CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
```

## Building

You will need to export the following environment variables in order to run `./build.sh`:

 - ANDROID_SDK_BUILD_TOOLS: path to android sdk build_tools
 - ANDROID_NDK_TOOLCHAIN: path to android ndk toolchain prebuilt sysroot
 - ANDROID_VERSION: the target android version number (must be >=22)
 - ANDROID_JAR: a path to the `android-XX.jar` file (see `pull_android_jar.sh`)
 - APP_NAME: the name of your app
 - ORG_NAME: the name of your organization
 - KEYSTORE_FILE: path to keystore file
 - STORE_PASS: keystore password
 - KEY_PASS: key password

The following environment variables are optional:
 - CFLAGS: common flags to be passed to C compiler
 - LDFLAGS: common flags to be passed to the linker

An example build command:
```
ANDROID_VERSION=30 ./pull_android_jar.sh
ANDROID_VERSION=30 \
    ANDROID_JAR=android-30.jar \
    ANDROID_SDK_BUILD_TOOLS=/home/user/android-sdk/build-tools/35.0.1 \
    ANDROID_NDK_TOOLCHAIN=/home/user/android-ndk/toolchains/llvm/prebuilt/linux-x86_64 \
    APP_NAME=seglapp \
    ORG_NAME=avensegl \
    KEYSTORE_FILE=./mykey.keystore \
    STORE_PASS=mypassword \
    KEY_PASS=mypassword \
    CFLAGS="-Wall -Wextra -Wno-unused-parameter -std=c11 -O3 -g0 -s" \
    ./build.sh
```

## Installing and testing

You will need to enable USB Debugging on the test device (or use an emulator) and then install the APK with `adb`:

```
adb install ./build_android/seglapp.apk # replace seglapp.apk with your app name
```

To view debug logs you can run:
```
adb shell logcat
```

To uninstall the app you can run:
```
adb uninstall avensegl.seglapp # replace avensegl and seglapp with your org and app names
```

[^1]: This may not be true with recent Android versions on some devices, but it
      is good to be accomodating.

[1]: https://github.com/cnlohr/rawdrawandroid
[2]: https://github.com/permutationlock/libavengraph
[3]: https://github.com/HomuHomu833/android-sdk-custom
[4]: https://github.com/HomuHomu833/android-ndk-custom

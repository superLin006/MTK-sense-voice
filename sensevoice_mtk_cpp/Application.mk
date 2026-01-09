# App ABI
APP_ABI := arm64-v8a

# Platform
APP_PLATFORM := android-24

# STL (使用 shared 以避免链接错误)
APP_STL := c++_shared

# CPP features
APP_CPPFLAGS := -std=c++11 -fexceptions -frtti

# Optimizations
APP_OPTIM := release

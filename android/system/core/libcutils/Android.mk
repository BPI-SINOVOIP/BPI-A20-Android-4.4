#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_CPU_SMP),true)
    targetSmpFlag := -DANDROID_SMP=1
else
    targetSmpFlag := -DANDROID_SMP=0
endif
hostSmpFlag := -DANDROID_SMP=0

commonSources := \
	hashmap.c \
	atomic.c.arm \
	native_handle.c \
	socket_inaddr_any_server.c \
	socket_local_client.c \
	socket_local_server.c \
	socket_loopback_client.c \
	socket_loopback_server.c \
	socket_network_client.c \
	sockets.c \
	config_utils.c \
	cpu_info.c \
	load_file.c \
	list.c \
	open_memstream.c \
	strdup16to8.c \
	strdup8to16.c \
	record_stream.c \
	process_name.c \
	threads.c \
	sched_policy.c \
	iosched_policy.c \
	str_parms.c \

commonHostSources := \
        ashmem-host.c

# some files must not be compiled when building against Mingw
# they correspond to features not used by our host development tools
# which are also hard or even impossible to port to native Win32
WINDOWS_HOST_ONLY :=
ifeq ($(HOST_OS),windows)
    ifeq ($(strip $(USE_CYGWIN)),)
        WINDOWS_HOST_ONLY := 1
    endif
endif
# USE_MINGW is defined when we build against Mingw on Linux
ifneq ($(strip $(USE_MINGW)),)
    WINDOWS_HOST_ONLY := 1
endif

ifneq ($(WINDOWS_HOST_ONLY),1)
    commonSources += \
        fs.c \
        multiuser.c
endif


# Static library for host
# ========================================================
LOCAL_MODULE := libcutils
LOCAL_SRC_FILES := $(commonSources) $(commonHostSources) dlmalloc_stubs.c
LOCAL_LDLIBS := -lpthread
LOCAL_STATIC_LIBRARIES := liblog
LOCAL_CFLAGS += $(hostSmpFlag)
include $(BUILD_HOST_STATIC_LIBRARY)


# Static library for host, 64-bit
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := lib64cutils
LOCAL_SRC_FILES := $(commonSources) $(commonHostSources) dlmalloc_stubs.c
LOCAL_LDLIBS := -lpthread
LOCAL_STATIC_LIBRARIES := lib64log
LOCAL_CFLAGS += $(hostSmpFlag) -m64
include $(BUILD_HOST_STATIC_LIBRARY)


# Shared and static library for target
# ========================================================

# This is needed in LOCAL_C_INCLUDES to access the C library's private
# header named <bionic_time.h>
#
libcutils_c_includes := bionic/libc/private

include $(CLEAR_VARS)
LOCAL_MODULE := libcutils
LOCAL_SRC_FILES := $(commonSources) \
        android_reboot.c \
        ashmem-dev.c \
        debugger.c \
        klog.c \
        partition_utils.c \
        properties.c \
        qtaguid.c \
        trace.c \
        uevent.c \
        misc_rw.c

ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES += arch-arm/memset32.S
else  # !arm
ifeq ($(TARGET_ARCH_VARIANT),x86-atom)
LOCAL_CFLAGS += -DHAVE_MEMSET16 -DHAVE_MEMSET32
LOCAL_SRC_FILES += arch-x86/android_memset16.S arch-x86/android_memset32.S memory.c
else # !x86-atom
ifeq ($(TARGET_ARCH),mips)
LOCAL_SRC_FILES += arch-mips/android_memset.c
else # !mips
LOCAL_SRC_FILES += memory.c
endif # !mips
endif # !x86-atom
endif # !arm

LOCAL_C_INCLUDES := $(libcutils_c_includes) $(KERNEL_HEADERS)
LOCAL_STATIC_LIBRARIES := liblog
LOCAL_CFLAGS += $(targetSmpFlag)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcutils
# TODO: remove liblog as whole static library, once we don't have prebuilt that requires
# liblog symbols present in libcutils.
LOCAL_WHOLE_STATIC_LIBRARIES := libcutils liblog
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_CFLAGS += $(targetSmpFlag)
LOCAL_C_INCLUDES := $(libcutils_c_includes)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tst_str_parms
LOCAL_CFLAGS += -DTEST_STR_PARMS
LOCAL_SRC_FILES := str_parms.c hashmap.c memory.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))

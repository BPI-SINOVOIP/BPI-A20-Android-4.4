# Copyright (C) 2010 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# suite/pts/hostTests/ptshostutil is treated specially
# as it cannot be put into ptscommonutilhost due to dependency on cts-tradefed
LOCAL_SRC_FILES := \
	$(call all-java-files-under, src) \
	$(call all-java-files-under, ../../suite/pts/hostTests/ptshostutil/src)

LOCAL_JAVA_RESOURCE_DIRS := res

LOCAL_MODULE := cts-tradefed
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LIBRARIES := ddmlib-prebuilt tradefed-prebuilt hosttestlib
LOCAL_STATIC_JAVA_LIBRARIES := ctsdeviceinfolib ptscommonutilhost

include $(BUILD_HOST_JAVA_LIBRARY)

# Build all sub-directories
include $(call all-makefiles-under,$(LOCAL_PATH))

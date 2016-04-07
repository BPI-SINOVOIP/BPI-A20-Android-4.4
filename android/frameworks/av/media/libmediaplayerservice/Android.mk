LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    Drm.cpp                     \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    SharedLibrary.cpp           \
    CedarPlayer.cpp             \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \
    CedarAPlayerWrapper.cpp	    \
    SimpleMediaFormatProbe.cpp  \
    MovAvInfoDetect.cpp         \
    MkvCodecInfoDetect.cpp

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    liblog                      \
    libdl                       \
    libgui                      \
    libmedia                    \
    libsonivox                  \
    libCedarX                   \
    libCedarA                   \
    libcedarxbase               \
    libcedarxosal               \
    libcedarv                   \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_httplive     \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libvorbisidec               \
    libui                       \

LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \
    libstagefright_matroska

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarXAndroid/IceCreamSandwich \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_audio \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_cedarv \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA/include \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \
    $(TOP)/frameworks/av/media/libstagefright/matroska

LOCAL_CFLAGS +=-DCEDARX_ANDROID_VERSION=11

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

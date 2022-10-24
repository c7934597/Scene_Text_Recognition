#-------------------------------------------------
#
# Project created by QtCreator 2016-08-08T09:18:55
#
#-------------------------------------------------

QT       -= core gui
CONFIG += c++14 plugin
TARGET = OCR_USDot
TEMPLATE = lib

DEFINES += PLEXUS_PLATEREC_LIBRARY
IS_ARM_64 = $$(ARM_64)

SOURCES += \
    ../main.cpp \
    ../OCR_USDot.cpp \
    ../../Filter.cpp \
    ../../OCR_Filter.cpp \
    ../../../Logging/LogUtil.cpp \
    ../../../Containers/PropertyBag.cpp \
    ../../../Video/Video.cpp \
    ../../../Containers/Buffer.cpp \
    ../../../Util/RapidJSON.cpp \
    ../../../Util/DateTimeUtil.cpp \
    ../../../Util/StringUtil.cpp \
    ../../../Video/FramerateController.cpp \
    ../../../Commands/HTTPCommand.cpp

HEADERS += \
    ../OCR_USDot.h \
    ../../Filter.h \
    ../../OCR_Filter.h \
    ../../../Logging/LogUtil.h \
    ../../../Containers/PropertyBag.h \
    ../../../Video/Video.h \
    ../../../Containers/Buffer.h \
    ../../../Util/RapidJSON.h \
    ../../../Util/DateTimeUtil.h \
    ../../../Util/StringUtil.h \
    ../../../Video/FramerateController.h \
    ../../../Commands/HTTPCommand.h

unix { QMAKE_CXXFLAGS += -Werror=return-type -Wno-return-type-c-linkage }

INCLUDEPATH += \
    ../../ \
    ../../../Common \
    ../../../Util \
    ../../../Containers \
    ../../../Logging \
    ../../../Video \
    ../../../LibInterfaceUtil \
    ../../../Events \
    ../../../Cryptography \
    ../../../Commands \
    $(PLEXUSLIB_DIR)/Linux/RapidJSON/include \
    $(CURL_DIR)/include \
    $(PLEXUSLIB_DIR)/Linux/deepstream/deepstream-6.0/sources/includes 

unix {
    !isEmpty(IS_ARM_64) {
        LIBS += -Wl,-z,defs -L$(PLEXUSLIB_DIR)/Linux/opencv_aarch64/bin/lib -lopencv_core -lopencv_objdetect -lopencv_dnn -lopencv_imgproc -lopencv_imgcodecs
        LIBS += -L$(CURL_DIR)/lib_aarch64 -lcurl
        LIBS += -L$(PLEXUSLIB_DIR)/Linux/deepstream/deepstream-6.0/lib/ -lnvdsgst_meta -lnvds_meta -lnvds_batch_jpegenc
        INCLUDEPATH += $(PLEXUSLIB_DIR)/Linux/opencv_aarch64/bin/include/opencv4
	INCLUDEPATH += /usr/local/cuda/include
	LIBS += -L/usr/local/cuda/lib64 -lcudart
    }
    else
    {
        LIBS += -Wl,-z,defs -L$(PLEXUSLIB_DIR)/Linux/opencv/bin/lib -lopencv_core -lopencv_objdetect -lopencv_dnn -lopencv_imgproc -lopencv_imgcodecs
        LIBS += -L$(CURL_DIR)/lib -lcurl
	    LIBS += -L$(PLEXUSLIB_DIR)/Linux/deepstream/deepstream-6.0/lib/ -lnvdsgst_meta -lnvds_meta -lnvds_batch_jpegenc
        INCLUDEPATH += $(PLEXUSLIB_DIR)/Linux/opencv/bin/include/opencv4
	INCLUDEPATH += /usr/local/cuda/include
	LIBS += -L/usr/local/cuda/lib64 -lcudart
    }

    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-base-1.0 gstreamer-app-1.0    

    CONFIG(debug, debug|release) {
        DESTDIR = "$(PLEXUS_DIR)/Libraries/Debug"
        LIBS += -ldl -L$(PLEXUS_DIR)/LibInterfaceUtil/Qt/bin/debug -lLibInterfaceUtil_D
    }
    CONFIG(release, debug|release) {
        CONFIG += force_debug_info
        CONFIG += separate_debug_info
        DESTDIR = "$(PLEXUS_DIR)/Libraries/Release"
        LIBS += -ldl -L$(PLEXUS_DIR)/LibInterfaceUtil/Qt/bin/release -lLibInterfaceUtil
    }
}

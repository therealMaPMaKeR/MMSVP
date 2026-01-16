QT       += core gui widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    vp_vlcplayer.cpp \
    lightweightvideoplayer.cpp

HEADERS += \
    vp_vlcplayer.h \
    lightweightvideoplayer.h

# LibVLC configuration for Windows
win32 {
    LIBVLC_PATH = $$PWD/3rdparty/libvlc

    CONFIG(debug, debug|release) {
        message("Debug build: Using libvlc")

        # Check if libvlc libraries exist
        !exists($$LIBVLC_PATH/lib/libvlc.lib) {
            warning("LibVLC libraries not found at $$LIBVLC_PATH/lib/")
            warning("Please copy libvlc.lib and libvlccore.lib to $$LIBVLC_PATH/lib/")
        }

        # Include libvlc headers
        INCLUDEPATH += $$LIBVLC_PATH/include

        # Link against libvlc libraries
        LIBS += -L$$LIBVLC_PATH/lib -llibvlc -llibvlccore

        # Copy VLC DLLs to output directory
        LIBVLC_DLLS = $$LIBVLC_PATH/bin/libvlc.dll $$LIBVLC_PATH/bin/libvlccore.dll

        for(dll, LIBVLC_DLLS) {
            exists($$dll) {
                QMAKE_POST_LINK += $$QMAKE_COPY "$$shell_path($$dll)" "$$shell_path($$OUT_PWD/debug)" $$escape_expand(\n\t)
            }
        }

        # Copy plugins directory to output
        PLUGIN_SOURCE = $$LIBVLC_PATH/bin/plugins
        PLUGIN_DEST = $$OUT_PWD/debug/plugins

        # Create plugins directory and copy all plugin files
        exists($$PLUGIN_SOURCE) {
            # Use xcopy on Windows to copy entire directory structure
            QMAKE_POST_LINK += $$QMAKE_MKDIR "$$shell_path($$PLUGIN_DEST)" $$escape_expand(\n\t)
            QMAKE_POST_LINK += xcopy /E /I /Y "$$shell_path($$PLUGIN_SOURCE)" "$$shell_path($$PLUGIN_DEST)" $$escape_expand(\n\t)
        }
    }

    CONFIG(release, debug|release) {
        message("Release build: Using libvlc")

        # Check if libvlc libraries exist
        !exists($$LIBVLC_PATH/lib/libvlc.lib) {
            warning("LibVLC libraries not found at $$LIBVLC_PATH/lib/")
            warning("Please copy libvlc.lib and libvlccore.lib to $$LIBVLC_PATH/lib/")
        }

        # Include libvlc headers
        INCLUDEPATH += $$LIBVLC_PATH/include

        # Link against libvlc libraries
        LIBS += -L$$LIBVLC_PATH/lib -llibvlc -llibvlccore

        # Copy VLC DLLs to output directory
        LIBVLC_DLLS = $$LIBVLC_PATH/bin/libvlc.dll $$LIBVLC_PATH/bin/libvlccore.dll

        for(dll, LIBVLC_DLLS) {
            exists($$dll) {
                QMAKE_POST_LINK += $$QMAKE_COPY "$$shell_path($$dll)" "$$shell_path($$OUT_PWD/release)" $$escape_expand(\n\t)
            }
        }

        # Copy plugins directory to output
        PLUGIN_SOURCE = $$LIBVLC_PATH/bin/plugins
        PLUGIN_DEST = $$OUT_PWD/release/plugins

        # Create plugins directory and copy all plugin files
        exists($$PLUGIN_SOURCE) {
            # Use xcopy on Windows to copy entire directory structure
            QMAKE_POST_LINK += $$QMAKE_MKDIR "$$shell_path($$PLUGIN_DEST)" $$escape_expand(\n\t)
            QMAKE_POST_LINK += xcopy /E /I /Y "$$shell_path($$PLUGIN_SOURCE)" "$$shell_path($$PLUGIN_DEST)" $$escape_expand(\n\t)
        }
    }

    # Define for conditional compilation
    DEFINES += USE_LIBVLC
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

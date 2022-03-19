cmake_minimum_required(VERSION 3.21)
project(porymap_master)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH C:\\Softwares\\Qt\\5.15.2\\mingw81_64\\lib\\cmake)

set(QT_VERSION 5)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(REQUIRED_LIBS Core Gui Widgets Concurrent Qml)
set(CMAKE_AUTOUIC_SEARCH_PATHS forms)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(QT5Qml REQUIRED)
find_package(QT5Concurrent REQUIRED)


include_directories(include)
include_directories(include/core)
include_directories(include/lib)
include_directories(include/ui)
include_directories(src/vendor/QtGifImage/3rdParty/giflib)
include_directories(src/vendor/QtGifImage/gifimage)
include_directories(forms)


add_executable(porymap_master
        src/core/block.cpp 
src/core/blockdata.cpp 
src/core/event.cpp 
src/core/heallocation.cpp 
src/core/imageexport.cpp 
src/core/map.cpp 
src/core/maplayout.cpp 
src/core/mapparser.cpp 
src/core/metatile.cpp 
src/core/metatileparser.cpp 
src/core/paletteutil.cpp 
src/core/parseutil.cpp 
src/core/tile.cpp 
src/core/tileset.cpp 
src/core/regionmap.cpp 
src/core/wildmoninfo.cpp 
src/core/editcommands.cpp 
src/lib/orderedjson.cpp 
src/mainwindow_scriptapi.cpp 
src/ui/aboutporymap.cpp 
src/ui/draggablepixmapitem.cpp 
src/ui/bordermetatilespixmapitem.cpp 
src/ui/collisionpixmapitem.cpp 
src/ui/connectionpixmapitem.cpp 
src/ui/currentselectedmetatilespixmapitem.cpp 
src/ui/overlay.cpp 
src/ui/regionmaplayoutpixmapitem.cpp 
src/ui/regionmapentriespixmapitem.cpp 
src/ui/cursortilerect.cpp 
src/ui/customattributestable.cpp 
src/ui/eventpropertiesframe.cpp 
src/ui/filterchildrenproxymodel.cpp 
src/ui/graphicsview.cpp 
src/ui/imageproviders.cpp 
src/ui/mappixmapitem.cpp 
src/ui/regionmappixmapitem.cpp 
src/ui/citymappixmapitem.cpp 
src/ui/mapsceneeventfilter.cpp 
src/ui/metatilelayersitem.cpp 
src/ui/metatileselector.cpp 
src/ui/movablerect.cpp 
src/ui/movementpermissionsselector.cpp 
src/ui/neweventtoolbutton.cpp 
src/ui/noscrollcombobox.cpp 
src/ui/noscrollspinbox.cpp 
src/ui/montabwidget.cpp 
src/ui/paletteeditor.cpp 
src/ui/selectablepixmapitem.cpp 
src/ui/tileseteditor.cpp 
src/ui/tileseteditormetatileselector.cpp 
src/ui/tileseteditortileselector.cpp 
src/ui/tilemaptileselector.cpp 
src/ui/regionmapeditor.cpp 
src/ui/newmappopup.cpp 
src/ui/mapimageexporter.cpp 
src/ui/newtilesetdialog.cpp 
src/ui/flowlayout.cpp 
src/ui/mapruler.cpp 
src/ui/shortcut.cpp 
src/ui/shortcutseditor.cpp 
src/ui/multikeyedit.cpp 
src/ui/preferenceeditor.cpp 
src/config.cpp 
src/editor.cpp 
src/main.cpp 
src/mainwindow.cpp 
src/project.cpp 
src/scripting.cpp 
src/settings.cpp 
src/log.cpp

include/core/block.h 
include/core/blockdata.h 
include/core/event.h 
include/core/heallocation.h 
include/core/history.h 
include/core/imageexport.h 
include/core/map.h 
include/core/mapconnection.h 
include/core/maplayout.h 
include/core/mapparser.h 
include/core/metatile.h 
include/core/metatileparser.h 
include/core/paletteutil.h 
include/core/parseutil.h 
include/core/tile.h 
include/core/tileset.h 
include/core/regionmap.h 
include/core/wildmoninfo.h 
include/core/editcommands.h 
include/lib/orderedmap.h 
include/lib/orderedjson.h 
include/ui/aboutporymap.h 
include/ui/draggablepixmapitem.h 
include/ui/bordermetatilespixmapitem.h 
include/ui/collisionpixmapitem.h 
include/ui/connectionpixmapitem.h 
include/ui/currentselectedmetatilespixmapitem.h 
include/ui/regionmaplayoutpixmapitem.h 
include/ui/regionmapentriespixmapitem.h 
include/ui/cursortilerect.h 
include/ui/customattributestable.h 
include/ui/eventpropertiesframe.h 
include/ui/filterchildrenproxymodel.h 
include/ui/graphicsview.h 
include/ui/imageproviders.h 
include/ui/mappixmapitem.h 
include/ui/regionmappixmapitem.h 
include/ui/citymappixmapitem.h 
include/ui/mapsceneeventfilter.h 
include/ui/metatilelayersitem.h 
include/ui/metatileselector.h 
include/ui/movablerect.h 
include/ui/movementpermissionsselector.h 
include/ui/neweventtoolbutton.h 
include/ui/noscrollcombobox.h 
include/ui/noscrollspinbox.h 
include/ui/montabwidget.h 
include/ui/adjustingstackedwidget.h 
include/ui/paletteeditor.h 
include/ui/selectablepixmapitem.h 
include/ui/tileseteditor.h 
include/ui/tileseteditormetatileselector.h 
include/ui/tileseteditortileselector.h 
include/ui/tilemaptileselector.h 
include/ui/regionmapeditor.h 
include/ui/newmappopup.h 
include/ui/mapimageexporter.h 
include/ui/newtilesetdialog.h 
include/ui/overlay.h 
include/ui/flowlayout.h 
include/ui/mapruler.h 
include/ui/shortcut.h 
include/ui/shortcutseditor.h 
include/ui/multikeyedit.h 
include/ui/preferenceeditor.h 
include/config.h 
include/editor.h 
include/mainwindow.h 
include/project.h 
include/scripting.h 
include/settings.h 
include/log.h

forms/mainwindow.ui 
forms/eventpropertiesframe.ui 
forms/tileseteditor.ui 
forms/paletteeditor.ui 
forms/regionmapeditor.ui 
forms/newmappopup.ui 
forms/aboutporymap.ui 
forms/newtilesetdialog.ui 
forms/mapimageexporter.ui 
forms/shortcutseditor.ui 
forms/preferenceeditor.ui

        resources/themes.qrc
        resources/images.qrc

        src/vendor/QtGifImage/3rdParty/giflib/dgif_lib.c
        src/vendor/QtGifImage/3rdParty/giflib/egif_lib.c
        src/vendor/QtGifImage/3rdParty/giflib/gif_err.c
        src/vendor/QtGifImage/3rdParty/giflib/gif_hash.c
        src/vendor/QtGifImage/3rdParty/giflib/gifalloc.c
        src/vendor/QtGifImage/3rdParty/giflib/quantize.c
        src/vendor/QtGifImage/3rdParty/giflib/gif_font.c
        src/vendor/QtGifImage/3rdParty/giflib/gif_hash.h
        src/vendor/QtGifImage/3rdParty/giflib/gif_lib.h
        src/vendor/QtGifImage/3rdParty/giflib/gif_lib_private.h

        src/vendor/QtGifImage/gifimage/qgifimage_p.h
        src/vendor/QtGifImage/gifimage/qgifimage.h
        src/vendor/QtGifImage/gifimage/qgifglobal.h
        src/vendor/QtGifImage/gifimage/qgifimage.cpp)


target_link_libraries(porymap_master Qt5::Qml)
target_link_libraries(porymap_master Qt5::Widgets)
target_link_libraries(porymap_master Qt5::Core)
target_link_libraries(porymap_master Qt5::Gui)
target_link_libraries(porymap_master Qt5::Concurrent)
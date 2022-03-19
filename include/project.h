#pragma once
#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"
#include "heallocation.h"
#include "event.h"
#include "wildmoninfo.h"
#include "parseutil.h"
#include "orderedjson.h"

#include <QStringList>
#include <QList>
#include <QVector>
#include <QPair>
#include <QStandardItem>
#include <QVariant>
#include <QFileSystemWatcher>

struct EventGraphics
{
    QImage spritesheet;
    int spriteWidth;
    int spriteHeight;
    bool inanimate;
};

static QString NONE_MAP_CONSTANT = "MAP_NONE";
static QString NONE_MAP_NAME = "None";

class Project : public QObject
{
    Q_OBJECT
public:
    Project(QWidget *parent = nullptr);
    ~Project();

    Project(const Project &) = delete;
    Project & operator = (const Project &) = delete;

    inline QWidget *parentWidget() const { return static_cast<QWidget *>(parent()); }

public:
    QString root;
    QStringList groupNames;
    QMap<QString, int> mapGroups;
    QList<QStringList> groupedMapNames;
    QStringList mapNames;
    QMap<QString, QVariant> miscConstants;
    QList<HealLocation> healLocations;
    QMap<QString, QString> mapConstantsToMapNames;
    QMap<QString, QString> mapNamesToMapConstants;
    QStringList mapLayoutsTable;
    QStringList mapLayoutsTableMaster;
    QString layoutsLabel;
    QMap<QString, MapLayout*> mapLayouts;
    QMap<QString, MapLayout*> mapLayoutsMaster;
    QMap<QString, QString> mapSecToMapHoverName;
    QMap<QString, int> mapSectionNameToValue;
    QMap<int, QString> mapSectionValueToName;
    QMap<QString, EventGraphics*> eventGraphicsMap;
    QMap<QString, int> gfxDefines;
    QStringList songNames;
    QStringList itemNames;
    QStringList flagNames;
    QStringList varNames;
    QStringList movementTypes;
    QStringList mapTypes;
    QStringList mapBattleScenes;
    QStringList weatherNames;
    QStringList coordEventWeatherNames;
    QStringList secretBaseIds;
    QStringList bgEventFacingDirections;
    QStringList trainerTypes;
    QStringList globalScriptLabels;
    QMap<QString, int> metatileBehaviorMap;
    QMap<int, QString> metatileBehaviorMapInverse;
    QMap<QString, QString> facingDirections;
    ParseUtil parser;
    QFileSystemWatcher fileWatcher;
    QMap<QString, qint64> modifiedFileTimestamps;

    void set_root(QString);

    void initSignals();

    void clearMapCache();
    void clearTilesetCache();

    struct DataQualifiers
    {
        bool isStatic;
        bool isConst;
    };
    DataQualifiers getDataQualifiers(QString, QString);
    QMap<QString, DataQualifiers> dataQualifiers;

    QMap<QString, Map*> mapCache;
    Map* loadMap(QString);
    Map* getMap(QString);

    QMap<QString, Tileset*> tilesetCache;
    Tileset* loadTileset(QString, Tileset *tileset = nullptr);
    Tileset* getTileset(QString, bool forceLoad = false);
    QMap<QString, QStringList> tilesetLabels;
    QList<QString> tilesetLabelsOrdered;

    Blockdata readBlockdata(QString);
    bool loadBlockdata(MapLayout*);
    bool loadLayoutBorder(MapLayout*);

    void saveTextFile(QString path, QString text);
    void appendTextFile(QString path, QString text);
    void deleteFile(QString path);

    bool readMapGroups();
    Map* addNewMapToGroup(QString, int, Map*, bool, bool);
    QString getNewMapName();
    QString getProjectTitle();

    QString readMapLayoutId(QString map_name);
    QString readMapLocation(QString map_name);

    bool readWildMonData();
    tsl::ordered_map<QString, tsl::ordered_map<QString, WildPokemonHeader>> wildMonData;

    QVector<EncounterField> wildMonFields;
    QVector<QString> encounterGroupLabels;
    QVector<poryjson::Json::object> extraEncounterGroups;

    bool readSpeciesIconPaths();
    QMap<QString, QString> speciesToIconPath;

    QMap<QString, bool> getTopLevelMapFields();
    bool loadMapData(Map*);
    bool readMapLayouts();
    bool loadLayout(MapLayout *);
    bool loadMapLayout(Map*);
    bool loadLayoutTilesets(MapLayout*);
    void loadTilesetAssets(Tileset*);
    void loadTilesetTiles(Tileset*, QImage);
    void loadTilesetMetatiles(Tileset*);
    void loadTilesetMetatileLabels(Tileset*);

    void saveLayoutBlockdata(Map*);
    void saveLayoutBorder(Map*);
    void writeBlockdata(QString, const Blockdata &);
    void saveAllMaps();
    void saveMap(Map*);
    void saveAllDataStructures();
    void saveMapLayouts();
    void saveMapGroups();
    void saveWildMonData();
    void saveMapConstantsHeader();
    void saveHealLocationStruct(Map*);
    void saveTilesets(Tileset*, Tileset*);
    void saveTilesetMetatileLabels(Tileset*, Tileset*);
    void saveTilesetMetatileAttributes(Tileset*);
    void saveTilesetMetatiles(Tileset*);
    void saveTilesetTilesImage(Tileset*);
    void saveTilesetPalettes(Tileset*);

    QString defaultSong;
    QStringList getVisibilities();
    QMap<QString, QStringList> getTilesetLabels();
    bool readTilesetProperties();
    bool readMaxMapDataSize();
    bool readRegionMapSections();
    bool readItemNames();
    bool readFlagNames();
    bool readVarNames();
    bool readMovementTypes();
    bool readInitialFacingDirections();
    bool readMapTypes();
    bool readMapBattleScenes();
    bool readWeatherNames();
    bool readCoordEventWeatherNames();
    bool readSecretBaseIds();
    bool readBgEventFacingDirections();
    bool readTrainerTypes();
    bool readMetatileBehaviors();
    bool readHealLocations();
    bool readMiscellaneousConstants();
    bool readEventScriptLabels();
    bool readObjEventGfxConstants();
    bool readSongNames();
    bool readEventGraphics();

    void setEventPixmap(Event * event, bool forceLoad = false);

    QString fixPalettePath(QString path);
    QString fixGraphicPath(QString path);

    QString getScriptFileExtension(bool usePoryScript) const;
    QString getScriptDefaultString(bool usePoryScript, QString mapName) const;
    QString getMapScriptsFilePath(const QString &mapName) const;
    QStringList getEventScriptsFilePaths() const;
    QCompleter *getEventScriptLabelCompleter(QStringList additionalScriptLabels);

    void saveMapHealEvents(Map *map);

    static int getNumTilesPrimary();
    static int getNumTilesTotal();
    static int getNumMetatilesPrimary();
    static int getNumMetatilesTotal();
    static int getNumPalettesPrimary();
    static int getNumPalettesTotal();
    static int getMaxMapDataSize();
    static int getDefaultMapSize();
    static int getMaxMapWidth();
    static int getMaxMapHeight();
    static int getMapDataSize(int width, int height);
    static bool mapDimensionsValid(int width, int height);
    bool calculateDefaultMapSize();
    static int getMaxObjectEvents();

private:
    void updateMapLayout(Map*);

    void setNewMapHeader(Map* map, int mapIndex);
    void setNewMapLayout(Map* map);
    void setNewMapBlockdata(Map* map);
    void setNewMapBorder(Map *map);
    void setNewMapEvents(Map *map);
    void setNewMapConnections(Map *map);

    void ignoreWatchedFileTemporarily(QString filepath);

    static int num_tiles_primary;
    static int num_tiles_total;
    static int num_metatiles_primary;
    static int num_metatiles_total;
    static int num_pals_primary;
    static int num_pals_total;
    static int max_map_data_size;
    static int default_map_size;
    static int max_object_events;

    QStringListModel eventScriptLabelModel;
    QCompleter eventScriptLabelCompleter;

signals:
    void reloadProject();
    void uncheckMonitorFilesAction();
    void mapCacheCleared();
};

#endif // PROJECT_H

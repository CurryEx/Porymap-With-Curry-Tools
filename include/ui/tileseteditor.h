#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "history.h"
#include "paletteeditor.h"
#include "tileseteditormetatileselector.h"
#include "tileseteditortileselector.h"
#include "metatilelayersitem.h"
#include "map.h"

namespace Ui {
class TilesetEditor;
}

class MetatileHistoryItem {
public:
    MetatileHistoryItem(uint16_t metatileId, Metatile *prevMetatile, Metatile *newMetatile) {
        this->metatileId = metatileId;
        this->prevMetatile = prevMetatile;
        this->newMetatile = newMetatile;
    }
    ~MetatileHistoryItem() {
        delete this->prevMetatile;
        delete this->newMetatile;
    }
    uint16_t metatileId;
    Metatile *prevMetatile;
    Metatile *newMetatile;
};

class TilesetEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit TilesetEditor(Project*, Map*, QWidget *parent = nullptr);
    ~TilesetEditor();
    void update(Map *map, QString primaryTilsetLabel, QString secondaryTilesetLabel);
    void updateMap(Map *map);
    void updateTilesets(QString primaryTilsetLabel, QString secondaryTilesetLabel);
    bool selectMetatile(uint16_t metatileId);
    uint16_t getSelectedMetatileId();
    void setMetatileLabel(QString label);

    QObjectList shortcutableObjects() const;

public slots:
    void applyUserShortcuts();

private slots:
    void onHoveredMetatileChanged(uint16_t);
    void onHoveredMetatileCleared();
    void onSelectedMetatileChanged(uint16_t);
    void onHoveredTileChanged(uint16_t);
    void onHoveredTileCleared();
    void onSelectedTilesChanged();
    void onMetatileLayerTileChanged(int, int);
    void onMetatileLayerSelectionChanged(QPoint, int, int);
    void onPaletteEditorChangedPaletteColor();
    void onPaletteEditorChangedPalette(int);

    void on_spinBox_paletteSelector_valueChanged(int arg1);

    void on_checkBox_xFlip_stateChanged(int arg1);

    void on_checkBox_yFlip_stateChanged(int arg1);

    void on_actionSave_Tileset_triggered();

    void on_actionImport_Primary_Tiles_triggered();

    void on_actionImport_Secondary_Tiles_triggered();

    void on_actionChange_Metatiles_Count_triggered();

    void on_actionChange_Palettes_triggered();

    void on_actionShow_Unused_toggled(bool checked);
    void on_actionShow_Counts_toggled(bool checked);
    void on_actionShow_UnusedTiles_toggled(bool checked);

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_comboBox_metatileBehaviors_textActivated(const QString &arg1);

    void on_lineEdit_metatileLabel_editingFinished();

    void on_comboBox_layerType_activated(int arg1);

    void on_comboBox_encounterType_activated(int arg1);

    void on_comboBox_terrainType_activated(int arg1);

    void on_actionExport_Primary_Tiles_Image_triggered();
    void on_actionExport_Secondary_Tiles_Image_triggered();
    void on_actionExport_Primary_Metatiles_Image_triggered();
    void on_actionExport_Secondary_Metatiles_Image_triggered();

    void on_actionImport_Primary_Metatiles_triggered();
    void on_actionImport_Secondary_Metatiles_triggered();

    void on_copyButton_metatileLabel_clicked();

    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();

    //@Curry
    void on_actionImportImageToTiles_triggered();

    void on_actionImportImageToMetatileReferToTiles_triggered();

    void on_actionExpandPrimaryTile_triggered();

    void on_actionExpandSecondaryTile_triggered();

    void on_actionExportPrimaryMetatilesToAM_triggered();

    void on_actionExportSecondaryMetatilesToAM_triggered();

    void on_actionExportCurrentPalttle_triggered();

private:
    void initUi();
    void setMetatileBehaviors();
    void setMetatileLayersUi();
    void setVersionSpecificUi();
    void setMetatileLabelValidator();
    void initMetatileSelector();
    void initTileSelector();
    void initSelectedTileItem();
    void initMetatileLayersItem();
    void initShortcuts();
    void initExtraShortcuts();
    void restoreWindowState();
    void initMetatileHistory();
    void setTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel);
    void reset();
    void drawSelectedTiles();
    void importTilesetTiles(Tileset*, bool);
    void importTilesetMetatiles(Tileset*, bool);
    void refresh();
    void saveMetatileLabel();
    void closeEvent(QCloseEvent*);
    void countMetatileUsage();
    void countTileUsage();
    void copyMetatile(bool cut);
    void pasteMetatile(const Metatile * toPaste);
    bool replaceMetatile(uint16_t metatileId, const Metatile * src);

    //Curry
    enum MatchResult {
        NOT_MATCH = 0,
        MATCH,
        X_FLIP,
        Y_FLIP,
        XY_FLIP
    };
    static MatchResult matchImage(const QImage& image1, const QImage& image2, bool allowFlip);
    QList<Tile> matchTile(const QImage& image);
    void expandTiles(bool isPrimary);
    void exportMetatilesToAM(bool isPrimary);
    QList<QImage> compressImage(const QImage& image, bool allowFlip, int widthPerBlock, int heightPerBlock, /*NULLABLE*/QJsonArray* outputSequence);
    class DialogResult{
    public:
        bool isAccept = false;
        bool allowEmpty = false;
        int width;
        int right;
    };
    DialogResult createParametersDialog(bool isMetatile, int selectedId, const QList<QImage>& compressedImageList);
    bool isGBAEmptyImage(const QImage& image);

    Ui::TilesetEditor *ui;
    History<MetatileHistoryItem*> metatileHistory;
    TilesetEditorMetatileSelector *metatileSelector = nullptr;
    TilesetEditorTileSelector *tileSelector = nullptr;
    MetatileLayersItem *metatileLayersItem = nullptr;
    PaletteEditor *paletteEditor = nullptr;
    Project *project = nullptr;
    Map *map = nullptr;
    Metatile *metatile = nullptr;
    Metatile *copiedMetatile = nullptr;
    int paletteId;
    bool tileXFlip;
    bool tileYFlip;
    bool hasUnsavedChanges;
    Tileset *primaryTileset = nullptr;
    Tileset *secondaryTileset = nullptr;
    QGraphicsScene *metatilesScene = nullptr;
    QGraphicsScene *tilesScene = nullptr;
    QGraphicsScene *selectedTileScene = nullptr;
    QGraphicsPixmapItem *selectedTilePixmapItem = nullptr;
    QGraphicsScene *metatileLayersScene = nullptr;

signals:
    void tilesetsSaved(QString, QString);
};

#endif // TILESETEDITOR_H

#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "log.h"
#include "imageproviders.h"
#include "metatileparser.h"
#include "paletteutil.h"
#include "imageexport.h"
#include "config.h"
#include "shortcut.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QImageReader>

TilesetEditor::TilesetEditor(Project *project, Map *map, QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::TilesetEditor),
        project(project),
        map(map),
        hasUnsavedChanges(false)
{
    this->setTilesets(this->map->layout->tileset_primary_label, this->map->layout->tileset_secondary_label);
    this->initUi();
    this->initMetatileHistory();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
    delete metatileSelector;
    delete tileSelector;
    delete metatileLayersItem;
    delete paletteEditor;
    delete metatile;
    delete primaryTileset;
    delete secondaryTileset;
    delete metatilesScene;
    delete tilesScene;
    delete selectedTilePixmapItem;
    delete selectedTileScene;
    delete metatileLayersScene;
}

void TilesetEditor::update(Map *map, QString primaryTilesetLabel, QString secondaryTilesetLabel)
{
    this->updateMap(map);
    this->updateTilesets(primaryTilesetLabel, secondaryTilesetLabel);
}

void TilesetEditor::updateMap(Map *map)
{
    this->map = map;
    this->metatileSelector->map = map;
}

void TilesetEditor::updateTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel)
{
    if (this->hasUnsavedChanges)
    {
        QMessageBox::StandardButton result = QMessageBox::question(
                this,
                "porymap",
                "Tileset has been modified, save changes?",
                QMessageBox::No | QMessageBox::Yes,
                QMessageBox::Yes);
        if (result == QMessageBox::Yes)
            this->on_actionSave_Tileset_triggered();
    }
    this->hasUnsavedChanges = false;
    this->setTilesets(primaryTilesetLabel, secondaryTilesetLabel);
    this->refresh();
}

bool TilesetEditor::selectMetatile(uint16_t metatileId)
{
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset)) return false;
    this->metatileSelector->select(metatileId);
    QPoint pos = this->metatileSelector->getMetatileIdCoordsOnWidget(metatileId);
    this->ui->scrollArea_Metatiles->ensureVisible(pos.x(), pos.y());
    return true;
}

uint16_t TilesetEditor::getSelectedMetatile()
{
    return this->metatileSelector->getSelectedMetatile();
}

void TilesetEditor::setTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel)
{
    Tileset *primaryTileset = project->getTileset(primaryTilesetLabel);
    Tileset *secondaryTileset = project->getTileset(secondaryTilesetLabel);
    if (this->primaryTileset) delete this->primaryTileset;
    if (this->secondaryTileset) delete this->secondaryTileset;
    this->primaryTileset = new Tileset(*primaryTileset);
    this->secondaryTileset = new Tileset(*secondaryTileset);
    if (paletteEditor) paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
}

void TilesetEditor::initUi()
{
    ui->setupUi(this);
    this->tileXFlip = ui->checkBox_xFlip->isChecked();
    this->tileYFlip = ui->checkBox_yFlip->isChecked();
    this->paletteId = ui->spinBox_paletteSelector->value();
    this->ui->spinBox_paletteSelector->setMinimum(0);
    this->ui->spinBox_paletteSelector->setMaximum(Project::getNumPalettesTotal() - 1);

    this->setMetatileBehaviors();
    this->setMetatileLayersUi();
    this->setVersionSpecificUi();
    this->setMetatileLabelValidator();

    this->initMetatileSelector();
    this->initMetatileLayersItem();
    this->initTileSelector();
    this->initSelectedTileItem();
    this->initShortcuts();
    this->metatileSelector->select(0);
    this->restoreWindowState();
}

void TilesetEditor::setMetatileBehaviors()
{
    for (int num: project->metatileBehaviorMapInverse.keys())
    {
        this->ui->comboBox_metatileBehaviors->addItem(project->metatileBehaviorMapInverse[num], num);
    }
}

void TilesetEditor::setMetatileLayersUi()
{
    if (!projectConfig.getTripleLayerMetatilesEnabled())
    {
        this->ui->comboBox_layerType->addItem("Normal - Middle/Top", METATILE_LAYER_MIDDLE_TOP);
        this->ui->comboBox_layerType->addItem("Covered - Bottom/Middle", METATILE_LAYER_BOTTOM_MIDDLE);
        this->ui->comboBox_layerType->addItem("Split - Bottom/Top", METATILE_LAYER_BOTTOM_TOP);
    }
    else
    {
        this->ui->comboBox_layerType->setVisible(false);
        this->ui->label_layerType->setVisible(false);
        this->ui->label_BottomTop->setText("Bottom/Middle/Top");
    }
}

void TilesetEditor::setVersionSpecificUi()
{
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered)
    {
        this->ui->comboBox_encounterType->setVisible(true);
        this->ui->label_encounterType->setVisible(true);
        this->ui->comboBox_encounterType->addItem("None", ENCOUNTER_NONE);
        this->ui->comboBox_encounterType->addItem("Land", ENCOUNTER_LAND);
        this->ui->comboBox_encounterType->addItem("Water", ENCOUNTER_WATER);
        this->ui->comboBox_terrainType->setVisible(true);
        this->ui->label_terrainType->setVisible(true);
        this->ui->comboBox_terrainType->addItem("Normal", TERRAIN_NONE);
        this->ui->comboBox_terrainType->addItem("Grass", TERRAIN_GRASS);
        this->ui->comboBox_terrainType->addItem("Water", TERRAIN_WATER);
        this->ui->comboBox_terrainType->addItem("Waterfall", TERRAIN_WATERFALL);
    }
    else
    {
        this->ui->comboBox_encounterType->setVisible(false);
        this->ui->label_encounterType->setVisible(false);
        this->ui->comboBox_terrainType->setVisible(false);
        this->ui->label_terrainType->setVisible(false);
    }
}

void TilesetEditor::setMetatileLabelValidator()
{
    //only allow characters valid for a symbol
    QRegularExpression expression("[_A-Za-z0-9]*$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    this->ui->lineEdit_metatileLabel->setValidator(validator);
}

void TilesetEditor::initMetatileSelector()
{
    this->metatileSelector = new TilesetEditorMetatileSelector(this->primaryTileset, this->secondaryTileset, this->map);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileChanged,
            this, &TilesetEditor::onHoveredMetatileChanged);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileCleared,
            this, &TilesetEditor::onHoveredMetatileCleared);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::selectedMetatileChanged,
            this, &TilesetEditor::onSelectedMetatileChanged);

    this->metatilesScene = new QGraphicsScene;
    this->metatilesScene->addItem(this->metatileSelector);
    this->metatileSelector->draw();

    this->ui->graphicsView_Metatiles->setScene(this->metatilesScene);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2,
                                                   this->metatileSelector->pixmap().height() + 2);
}

void TilesetEditor::initMetatileLayersItem()
{
    Metatile *metatile = Tileset::getMetatile(this->getSelectedMetatile(), this->primaryTileset,
                                              this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, &MetatileLayersItem::tileChanged,
            this, &TilesetEditor::onMetatileLayerTileChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::selectedTilesChanged,
            this, &TilesetEditor::onMetatileLayerSelectionChanged);

    this->metatileLayersScene = new QGraphicsScene;
    this->metatileLayersScene->addItem(this->metatileLayersItem);
    this->ui->graphicsView_metatileLayers->setScene(this->metatileLayersScene);
}

void TilesetEditor::initTileSelector()
{
    this->tileSelector = new TilesetEditorTileSelector(this->primaryTileset, this->secondaryTileset,
                                                       projectConfig.getTripleLayerMetatilesEnabled());
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileChanged,
            this, &TilesetEditor::onHoveredTileChanged);
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileCleared,
            this, &TilesetEditor::onHoveredTileCleared);
    connect(this->tileSelector, &TilesetEditorTileSelector::selectedTilesChanged,
            this, &TilesetEditor::onSelectedTilesChanged);

    this->tilesScene = new QGraphicsScene;
    this->tilesScene->addItem(this->tileSelector);
    this->tileSelector->select(0);
    this->tileSelector->draw();

    this->ui->graphicsView_Tiles->setScene(this->tilesScene);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2,
                                               this->tileSelector->pixmap().height() + 2);
}

void TilesetEditor::initSelectedTileItem()
{
    this->selectedTileScene = new QGraphicsScene;
    this->drawSelectedTiles();
    this->ui->graphicsView_selectedTile->setScene(this->selectedTileScene);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2,
                                                      this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::initShortcuts()
{
    initExtraShortcuts();

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();
}

void TilesetEditor::initExtraShortcuts()
{
    ui->actionRedo->setShortcuts({ui->actionRedo->shortcut(), QKeySequence("Ctrl+Shift+Z")});

    auto *shortcut_xFlip = new Shortcut(QKeySequence(), ui->checkBox_xFlip, SLOT(toggle()));
    shortcut_xFlip->setObjectName("shortcut_xFlip");
    shortcut_xFlip->setWhatsThis("X Flip");

    auto *shortcut_yFlip = new Shortcut(QKeySequence(), ui->checkBox_yFlip, SLOT(toggle()));
    shortcut_yFlip->setObjectName("shortcut_yFlip");
    shortcut_yFlip->setWhatsThis("Y Flip");
}

QObjectList TilesetEditor::shortcutableObjects() const
{
    QObjectList shortcutable_objects;

    for (auto *action: findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(action));
    for (auto *shortcut: findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(shortcut));

    return shortcutable_objects;
}

void TilesetEditor::applyUserShortcuts()
{
    for (auto *action: findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            action->setShortcuts(shortcutsConfig.userShortcuts(action));
    for (auto *shortcut: findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcut->setKeys(shortcutsConfig.userShortcuts(shortcut));
}

void TilesetEditor::restoreWindowState()
{
    logInfo("Restoring tileset editor geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getTilesetEditorGeometry();
    this->restoreGeometry(geometry.value("tileset_editor_geometry"));
    this->restoreState(geometry.value("tileset_editor_state"));
}

void TilesetEditor::initMetatileHistory()
{
    MetatileHistoryItem *commit = new MetatileHistoryItem(0, nullptr, new Metatile(*metatile));
    metatileHistory.push(commit);
}

void TilesetEditor::reset()
{
    this->hasUnsavedChanges = false;
    this->setTilesets(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor)
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->refresh();
}

void TilesetEditor::refresh()
{
    this->metatileLayersItem->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->tileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->select(this->getSelectedMetatile());
    this->drawSelectedTiles();

    if (metatileSelector)
    {
        if (metatileSelector->selectorShowUnused || metatileSelector->selectorShowCounts)
        {
            countMetatileUsage();
            this->metatileSelector->draw();
        }
    }

    if (tileSelector)
    {
        if (tileSelector->showUnused)
        {
            countTileUsage();
            this->tileSelector->draw();
        }
    }

    this->ui->graphicsView_Tiles->setSceneRect(0, 0, this->tileSelector->pixmap().width() + 2,
                                               this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2,
                                               this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setSceneRect(0, 0, this->metatileSelector->pixmap().width() + 2,
                                                   this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2,
                                                   this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2,
                                                      this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::drawSelectedTiles()
{
    if (!this->selectedTileScene)
    {
        return;
    }

    this->selectedTileScene->clear();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QImage selectionImage(16 * dimensions.x(), 16 * dimensions.y(), QImage::Format_RGBA8888);
    QPainter painter(&selectionImage);
    int tileIndex = 0;
    for (int j = 0; j < dimensions.y(); j++)
    {
        for (int i = 0; i < dimensions.x(); i++)
        {
            QImage tileImage = getPalettedTileImage(tiles.at(tileIndex).tileId, this->primaryTileset,
                                                    this->secondaryTileset, tiles.at(tileIndex).palette, true)
                    .mirrored(tiles.at(tileIndex).xflip, tiles.at(tileIndex).yflip)
                    .scaled(16, 16);
            tileIndex++;
            painter.drawImage(i * 16, j * 16, tileImage);
        }
    }

    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(selectionImage));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2,
                                                      this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::onHoveredMetatileChanged(uint16_t metatileId)
{
    Metatile *metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    QString message;
    QString hexString = QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper();
    if (metatile && metatile->label.size() != 0)
    {
        message = QString("Metatile: 0x%1 \"%2\"").arg(hexString, metatile->label);
    }
    else
    {
        message = QString("Metatile: 0x%1").arg(hexString);
    }
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredMetatileCleared()
{
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedMetatileChanged(uint16_t metatileId)
{
    this->metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem->setMetatile(metatile);
    this->metatileLayersItem->draw();
    this->ui->graphicsView_metatileLayers->setFixedSize(this->metatileLayersItem->pixmap().width() + 2,
                                                        this->metatileLayersItem->pixmap().height() + 2);
    this->ui->comboBox_metatileBehaviors->setCurrentIndex(
            this->ui->comboBox_metatileBehaviors->findData(this->metatile->behavior));
    this->ui->lineEdit_metatileLabel->setText(this->metatile->label);
    if (!projectConfig.getTripleLayerMetatilesEnabled())
    {
        this->ui->comboBox_layerType->setCurrentIndex(
                this->ui->comboBox_layerType->findData(this->metatile->layerType));
    }
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered)
    {
        this->ui->comboBox_encounterType->setCurrentIndex(
                this->ui->comboBox_encounterType->findData(this->metatile->encounterType));
        this->ui->comboBox_terrainType->setCurrentIndex(
                this->ui->comboBox_terrainType->findData(this->metatile->terrainType));
    }
}

void TilesetEditor::onHoveredTileChanged(uint16_t tile)
{
    QString message = QString("Tile: 0x%1")
            .arg(QString("%1").arg(tile, 3, 16, QChar('0')).toUpper());
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredTileCleared()
{
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedTilesChanged()
{
    this->drawSelectedTiles();
}

void TilesetEditor::onMetatileLayerTileChanged(int x, int y)
{
    static const QList<QPoint> tileCoords = QList<QPoint>{
            QPoint(0, 0),
            QPoint(1, 0),
            QPoint(0, 1),
            QPoint(1, 1),
            QPoint(2, 0),
            QPoint(3, 0),
            QPoint(2, 1),
            QPoint(3, 1),
            QPoint(4, 0),
            QPoint(5, 0),
            QPoint(4, 1),
            QPoint(5, 1),
    };
    Metatile *prevMetatile = new Metatile(*this->metatile);
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    int selectedTileIndex = 0;
    bool isTripleLayerMetatile = projectConfig.getTripleLayerMetatilesEnabled();
    int maxTileIndex = isTripleLayerMetatile ? 12 : 8;
    for (int j = 0; j < dimensions.y(); j++)
    {
        for (int i = 0; i < dimensions.x(); i++)
        {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < maxTileIndex
                && tileCoords.at(tileIndex).x() >= x
                && tileCoords.at(tileIndex).y() >= y)
            {
                Tile &tile = this->metatile->tiles[tileIndex];
                tile.tileId = tiles.at(selectedTileIndex).tileId;
                tile.xflip = tiles.at(selectedTileIndex).xflip;
                tile.yflip = tiles.at(selectedTileIndex).yflip;
                tile.palette = tiles.at(selectedTileIndex).palette;
            }
            selectedTileIndex++;
        }
    }

    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
    this->hasUnsavedChanges = true;

    MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                          prevMetatile, new Metatile(*this->metatile));
    metatileHistory.push(commit);
}

void TilesetEditor::onMetatileLayerSelectionChanged(QPoint selectionOrigin, int width, int height)
{
    QList<Tile> tiles;
    QList<int> tileIdxs;
    int x = selectionOrigin.x();
    int y = selectionOrigin.y();
    bool isTripleLayerMetatile = projectConfig.getTripleLayerMetatilesEnabled();
    int maxTileIndex = isTripleLayerMetatile ? 12 : 8;
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < maxTileIndex)
            {
                tiles.append(this->metatile->tiles.at(tileIndex));
                tileIdxs.append(tileIndex);
            }
        }
    }

    if (width == 1 && height == 1)
    {
        this->tileSelector->highlight(static_cast<uint16_t>(tiles[0].tileId));
        ui->spinBox_paletteSelector->setValue(tiles[0].palette);
        QPoint pos = tileSelector->getTileCoordsOnWidget(static_cast<uint16_t>(tiles[0].tileId));
        ui->scrollArea_Tiles->ensureVisible(pos.x(), pos.y());
    }
    this->tileSelector->setExternalSelection(width, height, tiles, tileIdxs);
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_spinBox_paletteSelector_valueChanged(int paletteId)
{
    this->ui->spinBox_paletteSelector->blockSignals(true);
    this->ui->spinBox_paletteSelector->setValue(paletteId);
    this->ui->spinBox_paletteSelector->blockSignals(false);
    this->paletteId = paletteId;
    this->tileSelector->setPaletteId(paletteId);
    this->drawSelectedTiles();
    if (this->paletteEditor)
    {
        this->paletteEditor->setPaletteId(paletteId);
    }
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_checkBox_xFlip_stateChanged(int checked)
{
    this->tileXFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_checkBox_yFlip_stateChanged(int checked)
{
    this->tileYFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_comboBox_metatileBehaviors_textActivated(const QString &metatileBehavior)
{
    if (this->metatile)
    {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->behavior = static_cast<uint16_t>(project->metatileBehaviorMap[metatileBehavior]);
        MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                              prevMetatile, new Metatile(*this->metatile));
        metatileHistory.push(commit);
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::setMetatileLabel(QString label)
{
    this->ui->lineEdit_metatileLabel->setText(label);
    saveMetatileLabel();
}

void TilesetEditor::on_lineEdit_metatileLabel_editingFinished()
{
    saveMetatileLabel();
}

void TilesetEditor::saveMetatileLabel()
{
    // Only commit if the field has changed.
    if (this->metatile && this->metatile->label != this->ui->lineEdit_metatileLabel->text())
    {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->label = this->ui->lineEdit_metatileLabel->text();
        MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                              prevMetatile, new Metatile(*this->metatile));
        metatileHistory.push(commit);
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_comboBox_layerType_activated(int layerType)
{
    if (this->metatile)
    {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->layerType = static_cast<uint8_t>(layerType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                              prevMetatile, new Metatile(*this->metatile));
        metatileHistory.push(commit);
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_comboBox_encounterType_activated(int encounterType)
{
    if (this->metatile)
    {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->encounterType = static_cast<uint8_t>(encounterType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                              prevMetatile, new Metatile(*this->metatile));
        metatileHistory.push(commit);
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_comboBox_terrainType_activated(int terrainType)
{
    if (this->metatile)
    {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->terrainType = static_cast<uint8_t>(terrainType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(this->getSelectedMetatile(),
                                                              prevMetatile, new Metatile(*this->metatile));
        metatileHistory.push(commit);
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_actionSave_Tileset_triggered()
{
    saveMetatileLabel();

    this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    emit this->tilesetsSaved(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor)
    {
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    }
    this->ui->statusbar->showMessage(QString("Saved primary and secondary Tilesets!"), 5000);
    this->hasUnsavedChanges = false;
}

void TilesetEditor::on_actionImport_Primary_Tiles_triggered()
{
    this->importTilesetTiles(this->primaryTileset, true);
}

void TilesetEditor::on_actionImport_Secondary_Tiles_triggered()
{
    this->importTilesetTiles(this->secondaryTileset, false);
}

void TilesetEditor::importTilesetTiles(Tileset *tileset, bool primary)
{
    QString descriptor = primary ? "primary" : "secondary";
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = QFileDialog::getOpenFileName(
            this,
            QString("Import %1 Tileset Tiles Image").arg(descriptorCaps),
            this->project->root,
            "Image Files (*.png *.bmp *.jpg *.dib)");
    if (filepath.isEmpty())
    {
        return;
    }

    logInfo(QString("Importing %1 tileset tiles '%2'").arg(descriptor).arg(filepath));

    // Read image data from buffer so that the built-in QImage doesn't try to detect file format
    // purely from the extension name. Advance Map exports ".png" files that are actually BMP format, for example.
    QFile file(filepath);
    QImage image;
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray imageData = file.readAll();
        image = QImage::fromData(imageData);
    }
    else
    {
        logError(QString("Failed to open image file: '%1'").arg(filepath));
    }
    if (image.width() == 0 || image.height() == 0 || image.width() % 8 != 0 || image.height() % 8 != 0)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(
                QString("The image dimensions (%1 x %2) are invalid. Width and height must be multiples of 8 pixels.")
                        .arg(image.width())
                        .arg(image.height()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Validate total number of tiles in image.
    int numTilesWide = image.width() / 8;
    int numTilesHigh = image.height() / 8;
    int totalTiles = numTilesHigh * numTilesWide;
    int maxAllowedTiles = primary ? Project::getNumTilesPrimary() : Project::getNumTilesTotal() -
                                                                    Project::getNumTilesPrimary();
    if (totalTiles > maxAllowedTiles)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(
                QString("The maximum number of tiles allowed in the %1 tileset is %2, but the provided image contains %3 total tiles.")
                        .arg(descriptor)
                        .arg(maxAllowedTiles)
                        .arg(totalTiles));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Ask user to provide a palette for the un-indexed image.
    if (image.colorCount() == 0)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Select Palette for Tiles");
        msgBox.setInformativeText(
                QString("The provided image is not indexed. Please select a palette file for the image. An indexed image will be generated using the provided image and palette.")
                        .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        msgBox.exec();

        QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Select Palette for Tiles Image").arg(descriptorCaps),
                this->project->root,
                "Palette Files (*.pal *.act *tpl *gpl)");
        if (filepath.isEmpty())
        {
            return;
        }

        bool error = false;
        QList<QRgb> palette = PaletteUtil::parse(filepath, &error);
        if (error)
        {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to import palette.");
            QString message = QString("The palette file could not be processed. View porymap.log for specific errors.");
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }
        else if (palette.length() != 16)
        {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to import palette.");
            QString message = QString("The palette must have exactly 16 colors, but it has %1.").arg(palette.length());
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }

        QVector<QRgb> colorTable = palette.toVector();
        image = image.convertToFormat(QImage::Format::Format_Indexed8, colorTable);
    }

    // Validate image is properly indexed to 16 colors.
    if (image.colorCount() != 16)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(
                QString("The image must be indexed and contain 16 total colors, or it must be un-indexed. The provided image has %1 indexed colors.")
                        .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    this->project->loadTilesetTiles(tileset, image);
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::closeEvent(QCloseEvent *event)
{
    if (this->hasUnsavedChanges)
    {
        QMessageBox::StandardButton result = QMessageBox::question(
                this,
                "porymap",
                "Tileset has been modified, save changes?",
                QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Yes);

        if (result == QMessageBox::Yes)
        {
            this->on_actionSave_Tileset_triggered();
            event->accept();
        }
        else if (result == QMessageBox::No)
        {
            this->reset();
            event->accept();
        }
        else if (result == QMessageBox::Cancel)
        {
            event->ignore();
        }
    }
    else
    {
        event->accept();
    }

    if (event->isAccepted())
    {
        if (this->paletteEditor) this->paletteEditor->close();
        porymapConfig.setTilesetEditorGeometry(
                this->saveGeometry(),
                this->saveState()
        );
    }
}

void TilesetEditor::on_actionChange_Metatiles_Count_triggered()
{
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Change Number of Metatiles");
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);

    QSpinBox *primarySpinBox = new QSpinBox();
    QSpinBox *secondarySpinBox = new QSpinBox();
    primarySpinBox->setMinimum(1);
    secondarySpinBox->setMinimum(1);
    primarySpinBox->setMaximum(Project::getNumMetatilesPrimary());
    secondarySpinBox->setMaximum(Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary());
    primarySpinBox->setValue(this->primaryTileset->metatiles.length());
    secondarySpinBox->setValue(this->secondaryTileset->metatiles.length());
    form.addRow(new QLabel("Primary Tileset"), primarySpinBox);
    form.addRow(new QLabel("Secondary Tileset"), secondarySpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted)
    {
        int numPrimaryMetatiles = primarySpinBox->value();
        int numSecondaryMetatiles = secondarySpinBox->value();
        int numTiles = projectConfig.getTripleLayerMetatilesEnabled() ? 12 : 8;
        while (this->primaryTileset->metatiles.length() > numPrimaryMetatiles)
        {
            delete this->primaryTileset->metatiles.takeLast();
        }
        while (this->primaryTileset->metatiles.length() < numPrimaryMetatiles)
        {
            Tile tile(0, false, false, 0);
            Metatile *metatile = new Metatile();
            for (int i = 0; i < numTiles; i++)
            {
                metatile->tiles.append(tile);
            }
            this->primaryTileset->metatiles.append(metatile);
        }
        while (this->secondaryTileset->metatiles.length() > numSecondaryMetatiles)
        {
            delete this->secondaryTileset->metatiles.takeLast();
        }
        while (this->secondaryTileset->metatiles.length() < numSecondaryMetatiles)
        {
            Tile tile(0, false, false, 0);
            Metatile *metatile = new Metatile();
            for (int i = 0; i < numTiles; i++)
            {
                metatile->tiles.append(tile);
            }
            this->secondaryTileset->metatiles.append(metatile);
        }

        this->metatileSelector->updateSelectedMetatile();
        this->refresh();
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_actionChange_Palettes_triggered()
{
    if (!this->paletteEditor)
    {
        this->paletteEditor = new PaletteEditor(this->project, this->primaryTileset,
                                                this->secondaryTileset, this->paletteId, this);
        connect(this->paletteEditor, &PaletteEditor::changedPaletteColor,
                this, &TilesetEditor::onPaletteEditorChangedPaletteColor);
        connect(this->paletteEditor, &PaletteEditor::changedPalette,
                this, &TilesetEditor::onPaletteEditorChangedPalette);
    }

    if (!this->paletteEditor->isVisible())
    {
        this->paletteEditor->show();
    }
    else if (this->paletteEditor->isMinimized())
    {
        this->paletteEditor->showNormal();
    }
    else
    {
        this->paletteEditor->activateWindow();
    }
}

void TilesetEditor::onPaletteEditorChangedPaletteColor()
{
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::onPaletteEditorChangedPalette(int paletteId)
{
    this->on_spinBox_paletteSelector_valueChanged(paletteId);
}

void TilesetEditor::on_actionUndo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.current();
    if (!commit) return;
    Metatile *prev = commit->prevMetatile;
    if (!prev) return;
    this->metatileHistory.back();

    Metatile *temp = Tileset::getMetatile(commit->metatileId, this->primaryTileset, this->secondaryTileset);
    if (temp)
    {
        this->metatile = temp;
        *this->metatile = *prev;
        this->metatileSelector->select(commit->metatileId);
        this->metatileSelector->draw();
        this->metatileLayersItem->draw();
        this->metatileLayersItem->clearLastModifiedCoords();
    }
}

void TilesetEditor::on_actionRedo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.next();
    if (!commit) return;
    Metatile *next = commit->newMetatile;
    if (!next) return;

    Metatile *temp = Tileset::getMetatile(commit->metatileId, this->primaryTileset, this->secondaryTileset);
    if (temp)
    {
        this->metatile = temp;
        *this->metatile = *next;
        this->metatileSelector->select(commit->metatileId);
        this->metatileSelector->draw();
        this->metatileLayersItem->draw();
        this->metatileLayersItem->clearLastModifiedCoords();
    }
}

void TilesetEditor::on_actionExport_Primary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->primaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->root).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Primary Tiles Image", defaultFilepath,
                                                    "Image Files (*.png)");
    if (!filepath.isEmpty())
    {
        QImage image = this->tileSelector->buildPrimaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

void TilesetEditor::on_actionExport_Secondary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->secondaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->root).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Secondary Tiles Image", defaultFilepath,
                                                    "Image Files (*.png)");
    if (!filepath.isEmpty())
    {
        QImage image = this->tileSelector->buildSecondaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

void TilesetEditor::on_actionImport_Primary_Metatiles_triggered()
{
    this->importTilesetMetatiles(this->primaryTileset, true);
}

void TilesetEditor::on_actionImport_Secondary_Metatiles_triggered()
{
    this->importTilesetMetatiles(this->secondaryTileset, false);
}

void TilesetEditor::importTilesetMetatiles(Tileset *tileset, bool primary)
{
    QString descriptor = primary ? "primary" : "secondary";
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = QFileDialog::getOpenFileName(
            this,
            QString("Import %1 Tileset Metatiles from Advance Map 1.92").arg(descriptorCaps),
            this->project->root,
            "Advance Map 1.92 Metatile Files (*.bvd)");
    if (filepath.isEmpty())
    {
        return;
    }

    bool error = false;
    QList<Metatile *> metatiles = MetatileParser::parse(filepath, &error, primary);
    if (error)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import metatiles from Advance Map 1.92 .bvd file.");
        QString message = QString("The .bvd file could not be processed. View porymap.log for specific errors.");
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // TODO: This is crude because it makes a history entry for every newly-imported metatile.
    //       Revisit this when tiles and num metatiles are added to tileset editory history.
    int metatileIdBase = primary ? 0 : Project::getNumMetatilesPrimary();
    for (int i = 0; i < metatiles.length(); i++)
    {
        if (i >= tileset->metatiles.length())
        {
            break;
        }

        Metatile *prevMetatile = new Metatile(*tileset->metatiles.at(i));
        MetatileHistoryItem *commit = new MetatileHistoryItem(static_cast<uint16_t>(metatileIdBase + i),
                                                              prevMetatile, new Metatile(*metatiles.at(i)));
        metatileHistory.push(commit);
    }

    tileset->metatiles = metatiles;
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::on_actionShow_Unused_toggled(bool checked)
{
    this->metatileSelector->selectorShowUnused = checked;

    if (checked) countMetatileUsage();

    this->metatileSelector->draw();
}

void TilesetEditor::on_actionShow_Counts_toggled(bool checked)
{
    this->metatileSelector->selectorShowCounts = checked;

    if (checked) countMetatileUsage();

    this->metatileSelector->draw();
}

void TilesetEditor::on_actionShow_UnusedTiles_toggled(bool checked)
{
    this->tileSelector->showUnused = checked;

    if (checked) countTileUsage();

    this->tileSelector->draw();
}

void TilesetEditor::countMetatileUsage()
{
    // do not double count
    metatileSelector->usedMetatiles.fill(0);

    for (auto layout: this->project->mapLayouts.values())
    {
        bool usesPrimary = false;
        bool usesSecondary = false;

        if (layout->tileset_primary_label == this->primaryTileset->name)
        {
            usesPrimary = true;
        }

        if (layout->tileset_secondary_label == this->secondaryTileset->name)
        {
            usesSecondary = true;
        }

        if (usesPrimary || usesSecondary)
        {
            this->project->loadLayout(layout);

            // for each block in the layout, mark in the vector that it is used
            for (int i = 0; i < layout->blockdata.length(); i++)
            {
                uint16_t metatileId = layout->blockdata.at(i).metatileId;
                if (metatileId < this->project->getNumMetatilesPrimary())
                {
                    if (usesPrimary) metatileSelector->usedMetatiles[metatileId]++;
                }
                else
                {
                    if (usesSecondary) metatileSelector->usedMetatiles[metatileId]++;
                }
            }

            for (int i = 0; i < layout->border.length(); i++)
            {
                uint16_t metatileId = layout->border.at(i).metatileId;
                if (metatileId < this->project->getNumMetatilesPrimary())
                {
                    if (usesPrimary) metatileSelector->usedMetatiles[metatileId]++;
                }
                else
                {
                    if (usesSecondary) metatileSelector->usedMetatiles[metatileId]++;
                }
            }
        }
    }
}

void TilesetEditor::countTileUsage()
{
    // check primary tiles
    this->tileSelector->usedTiles.resize(Project::getNumTilesTotal());
    this->tileSelector->usedTiles.fill(0);

    QSet<Tileset *> primaryTilesets;
    QSet<Tileset *> secondaryTilesets;

    for (auto layout: this->project->mapLayouts.values())
    {
        if (layout->tileset_primary_label == this->primaryTileset->name
            || layout->tileset_secondary_label == this->secondaryTileset->name)
        {
            this->project->loadLayoutTilesets(layout);
            // need to check metatiles
            if (layout->tileset_primary && layout->tileset_secondary)
            {
                primaryTilesets.insert(layout->tileset_primary);
                secondaryTilesets.insert(layout->tileset_secondary);
            }
        }
    }

    // check primary tilesets that are used with this secondary tileset for
    // reference to secondary tiles in primary metatiles
    for (Tileset *tileset: primaryTilesets)
    {
        for (Metatile *metatile: tileset->metatiles)
        {
            for (Tile tile: metatile->tiles)
            {
                if (tile.tileId >= Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // do the opposite for primary tiles in secondary metatiles
    for (Tileset *tileset: secondaryTilesets)
    {
        for (Metatile *metatile: tileset->metatiles)
        {
            for (Tile tile: metatile->tiles)
            {
                if (tile.tileId < Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // check this primary tileset metatiles
    for (Metatile *metatile: this->primaryTileset->metatiles)
    {
        for (Tile tile: metatile->tiles)
        {
            this->tileSelector->usedTiles[tile.tileId]++;
        }
    }

    // and the secondary metatiles
    for (Metatile *metatile: this->secondaryTileset->metatiles)
    {
        for (Tile tile: metatile->tiles)
        {
            this->tileSelector->usedTiles[tile.tileId]++;
        }
    }
}

//@Curry
//?????????????????????tile
void TilesetEditor::on_actionImportImageToTiles_triggered()
{
    //?????????????????????
    QMessageBox msgBox(this);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setText(tr("??????"));

    //?????????????????????tile
    int selectedTileId = this->tileSelector->getSelectedTiles().at(0).tileId;
    // selectedTileId < this->primaryTileset->tiles.count() ? true : false
    bool primary = selectedTileId < this->primaryTileset->tiles.count();

    //todo ?????????t0??????????????????????????????????????? ???????????????????????????
    if (!primary)
        if (Project::getNumTilesPrimary() != this->primaryTileset->tiles.count())
        {
            msgBox.setInformativeText(tr("????????????PrimaryTiles????????? %1 ??????????????????????????? %2 ???????????????secondary tiles??????????????? %3")
                                              .arg(this->primaryTileset->tiles.count())
                                              .arg(Project::getNumTilesPrimary())
                                              .arg(Project::getNumTilesPrimary()));
            msgBox.exec();
            return;
        }

    //????????????
    QString filepath = QFileDialog::getOpenFileName(
            this,
            tr("????????????"),
            this->project->root,
            tr("?????? (*.png *.bmp *.jpg *.dib)"));
    if (filepath.isEmpty())
    {
        return;
    }

    //????????????
    QFile file(filepath);
    QImage image;
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray imageData = file.readAll();
        image = QImage::fromData(imageData);
    }
    else
    {
        msgBox.setInformativeText(tr("?????????????????? ???????????????"));
        msgBox.exec();
        return;
    }
    if (image.width() == 0 || image.height() == 0 || image.width() % 8 != 0 || image.height() % 8 != 0)
    {
        msgBox.setInformativeText(
                tr("????????? ?????? (%1 x %2) ????????????8??????")
                        .arg(image.width())
                        .arg(image.height()));
        msgBox.exec();
        return;
    }
    if (image.colorCount() != 16)
    {
        msgBox.setInformativeText(tr("????????????????????????16???"));
        msgBox.exec();
        return;
    }

    //??????
    auto compressedTiles = compressImage(image, true, 8, 8, nullptr);

    auto result = createParametersDialog(false, selectedTileId, compressedTiles);
    if (result.isAccept)
    {
        int exceptWidth = result.width;
        int right = result.right;
        bool allowEmpty = result.allowEmpty;

        //????????????????????????
        if (right != 0)
        {
            if (right >= exceptWidth)
            {
                msgBox.setInformativeText(tr("????????????"));
                msgBox.exec();
                return;
            }
        }
        if (selectedTileId % 16 + exceptWidth > 16)
        {
            msgBox.setInformativeText(tr("????????????"));
            msgBox.exec();
            return;
        }

        //????????????????????????
        int selectedHeight;
        QList<QImage>* targetTiles;
        if (primary)
        {
            targetTiles = &this->primaryTileset->tiles;
        }
        else
        {
            selectedTileId -= this->primaryTileset->tiles.count();
            targetTiles = &this->secondaryTileset->tiles;
        }

        //?????????????????????????????????????????????
        selectedHeight = (selectedTileId - (selectedTileId % 16)) / 16;
        int allowHeight = (targetTiles->count() / 16) - selectedHeight;

        //???????????????????????????????????????????????????right?????????????????????
        int allowBlock;
        if (right != 0)
            allowBlock = exceptWidth * allowHeight - right;
        else
            allowBlock = exceptWidth * allowHeight;

        if (allowBlock < compressedTiles.count())
        {
            msgBox.setInformativeText(tr("???????????? ??????????????????????????? %1 ??? ??????????????? %2 ???")
                                              .arg(allowBlock)
                                              .arg(compressedTiles.count()));
            msgBox.exec();
            return;
        }

        //?????????????????????
        auto *progressDialog = new QProgressDialog(this);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(nullptr);
        progressDialog->setWindowTitle(tr("?????????"));
        progressDialog->setLabelText(tr(
                "1.??????????????? ??????????????? ?????????????????????????????? ????????????\n\n"
                "2.???????????? ???????????????????????? ?????????????????? ??????????????? ???????????????????????????\n\n"
                "??????Java ????????????"));
        progressDialog->setRange(0, compressedTiles.count());

        //???????????? ??????????????????
        int offset = 0;
        int nowY = 0;
        if (right != 0)
        {
            nowY += right;
            selectedTileId += right;
        }
        for (int i = 0; i < compressedTiles.count(); i++)
        {
            QApplication::processEvents();
            //??????????????????????????????
            if(!allowEmpty)
            {
                if(isGBAEmptyImage(compressedTiles.at(i)))
                {
                    offset--;
                    continue;
                }
            }
            progressDialog->setValue(i);
            int nowProcess = selectedTileId + i + offset;
            if (++nowY == exceptWidth)
            {
                nowY = 0;
                offset = offset + 16 - exceptWidth;
            }
            targetTiles->replace(nowProcess, compressedTiles.at(i));
        }

        this->primaryTileset->tilesImage = this->tileSelector->buildPrimaryTilesIndexedImage();
        this->secondaryTileset->tilesImage = this->tileSelector->buildSecondaryTilesIndexedImage();
        this->refresh();
        this->hasUnsavedChanges = true;
        progressDialog->close();
    }
}

TilesetEditor::MatchResult TilesetEditor::matchImage(const QImage &image1, const QImage &image2, bool allowFlip)
{
    int width = image1.width();
    int height = image1.height();

    if (width != image2.width() || height != image2.height())
        return MatchResult::NOT_MATCH;

    bool success = true;
    //??????
    for (int yCompIdx = 0; yCompIdx < height; yCompIdx++)
    {
        for (int xCompIdx = 0; xCompIdx < width; xCompIdx++)
        {
            if (image1.pixelColor(xCompIdx, yCompIdx) !=
                image2.pixelColor(xCompIdx, yCompIdx))
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }
    if (success)
    {
        return MatchResult::MATCH;
    }
    else
    {
        if (!allowFlip)
            return MatchResult::NOT_MATCH;
        else
            success = true;
    }

    for (int yCompIdx = 0; yCompIdx < height; yCompIdx++)
    {
        for (int xCompIdx = 0; xCompIdx < width; xCompIdx++)
        {
            if (image1.pixelColor(xCompIdx, yCompIdx) !=
                image2.pixelColor(width - 1 - xCompIdx, yCompIdx))
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }
    if (success)
    {
        return MatchResult::X_FLIP;
    }
    else
    {
        success = true;
    }

    for (int yCompIdx = 0; yCompIdx < height; yCompIdx++)
    {
        for (int xCompIdx = 0; xCompIdx < width; xCompIdx++)
        {
            if (image1.pixelColor(xCompIdx, yCompIdx) !=
                image2.pixelColor(xCompIdx, height - 1 - yCompIdx))
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }
    if (success)
    {
        return MatchResult::Y_FLIP;
    }
    else
    {
        success = true;
    }

    for (int yCompIdx = 0; yCompIdx < height; yCompIdx++)
    {
        for (int xCompIdx = 0; xCompIdx < width; xCompIdx++)
        {
            if (image1.pixelColor(xCompIdx, yCompIdx) !=
                image2.pixelColor(width - 1 - xCompIdx, height - 1 - yCompIdx))
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }
    if (success)
    {
        return MatchResult::XY_FLIP;
    }
    else
    {
        return MatchResult::NOT_MATCH;
    }
}

void TilesetEditor::on_actionImportImageToMetatileReferToTiles_triggered()
{
    int selectedMetatileId = this->metatileSelector->getSelectedMetatile();
    int secondaryMetatileCount = this->secondaryTileset->metatiles.length();
    int primaryMetatileCount = this->primaryTileset->metatiles.length();

    //?????????????????? ??????????????????
    QMessageBox msgBox(this);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Critical);

    // ???????????????meta???prim???
    // selectedMetatileId >= primaryMetatileCount ? false : true
    bool primary = selectedMetatileId < primaryMetatileCount;
    int maxAllowedTiles = primary ?
                          primaryMetatileCount - selectedMetatileId :
                          secondaryMetatileCount - (selectedMetatileId - primaryMetatileCount);

    if (!primary)
        if (Project::getNumMetatilesPrimary() != this->primaryTileset->metatiles.count())
        {
            msgBox.setInformativeText(tr("????????????PrimaryMetatiles????????? %1 ??????????????????????????? %2 ???????????????secondary metatile??????????????? %3")
                                              .arg(this->primaryTileset->metatiles.count())
                                              .arg(Project::getNumMetatilesPrimary())
                                              .arg(Project::getNumMetatilesPrimary()));
            msgBox.exec();
            return;
        }

    QMessageBox confirm(this);
    confirm.setText(tr("??????"));
    confirm.setInformativeText(
            tr("???????????????\n\n"
                    "1.??????????????????????????????????????????????????????\n\n"
                    "2.???????????????Metatile?????????????????????\n\n"
                    "3.???????????????Metatile????????????????????????Metatile?????? ??????????????????????????????\n\n"));
    auto cancel = confirm.addButton(tr("????????????"), QMessageBox::ActionRole);
    confirm.addButton(tr("??????"), QMessageBox::ActionRole);
    confirm.setIcon(QMessageBox::Icon::Information);
    confirm.exec();
    if (confirm.clickedButton() == cancel)
        return;

    QString filepath = QFileDialog::getOpenFileName(
            this,
            tr("????????????"),
            this->project->root,
            tr("?????? (*.png *.bmp *.jpg *.dib)"));
    if (filepath.isEmpty())
    {
        return;
    }

    //????????????
    QFile file(filepath);
    QImage image;
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray imageData = file.readAll();
        image = QImage::fromData(imageData);
    }
    else
    {
        msgBox.setText(tr("??????"));
        msgBox.setInformativeText(tr("??????????????????"));
        msgBox.exec();
        return;
    }
    if (image.width() == 0 || image.height() == 0 || image.width() % 16 != 0 || image.height() % 16 != 0)
    {
        msgBox.setText(tr("??????"));
        msgBox.setInformativeText(
                tr("???????????? ?????? (%1 x %2) ????????????16??????")
                        .arg(image.width())
                        .arg(image.height()));
        msgBox.exec();
        return;
    }


//    logInfo(QString("primaryMetatileCount %1 secondaryMetatileCount %2 this->currentSelectId %3 maxAllowedTiles %4")
//                    .arg(primaryMetatileCount)
//                    .arg(secondaryMetatileCount)
//                    .arg(selectedMetatileId)
//                    .arg(maxAllowedTiles)
//    );

    //????????????????????????
    int numTilesWide = image.width() / 16;
    int numTilesHigh = image.height() / 16;
    int totalTiles = numTilesHigh * numTilesWide;

    //??????????????????????????????
    QJsonArray sequence;
    //??????
    auto compressedMetatiles = compressImage(image, false, 16, 16, &sequence);

    auto result = createParametersDialog(true, selectedMetatileId, compressedMetatiles);
    if (result.isAccept)
    {
        //????????????????????????
        bool isTripleLayerMetatile = projectConfig.getTripleLayerMetatilesEnabled();
        int topTiles[4];
        if (isTripleLayerMetatile)
        {
            topTiles[0] = 8;
            topTiles[1] = 9;
            topTiles[2] = 10;
            topTiles[3] = 11;
        }
        else
        {
            topTiles[0] = 4;
            topTiles[1] = 5;
            topTiles[2] = 6;
            topTiles[3] = 7;
        }

        //????????????????????????
        int exceptWidth = result.width;
        int right = result.right;
        bool allowEmpty = result.allowEmpty;
        if (right != 0)
        {
            if (right >= exceptWidth)
            {
                msgBox.setInformativeText(tr("????????????"));
                msgBox.exec();
                return;
            }
        }
        if (selectedMetatileId % 8 + exceptWidth > 8)
        {
            msgBox.setInformativeText(tr("????????????"));
            msgBox.exec();
            return;
        }

        //????????????????????????
        int selectedHeight;
        QList<Metatile *>* targetMetatiles;
        if (primary)
        {
            //?????????????????????????????????????????????
            selectedHeight = (selectedMetatileId - (selectedMetatileId % 8)) / 8;
            targetMetatiles = &this->primaryTileset->metatiles;
        }
        else
        {
            //????????????????????????????????????????????? ???????????????????????? ????????????
            int selectedMetatileId_ = selectedMetatileId - this->primaryTileset->metatiles.count();
            selectedHeight = (selectedMetatileId_ - (selectedMetatileId_ % 8)) / 8;
            targetMetatiles = &this->secondaryTileset->metatiles;
        }
        int allowHeight = (targetMetatiles->count() / 8) - selectedHeight;

        //???????????????????????????????????????????????????right?????????????????????
        int allowBlock;
        if (right != 0)
            allowBlock = exceptWidth * allowHeight - right;
        else
            allowBlock = exceptWidth * allowHeight;

        if (allowBlock < compressedMetatiles.count())
        {
            msgBox.setInformativeText(tr("???????????? ??????????????????????????? %1 ??? ??????????????? %2 ???")
                                              .arg(allowBlock)
                                              .arg(compressedMetatiles.count()));
            msgBox.exec();
            return;
        }

        //?????????????????????
        auto *progressDialog = new QProgressDialog(this);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(nullptr);
        progressDialog->setWindowTitle(tr("?????????"));
        progressDialog->setLabelText(tr(
                "1.??????????????? ??????????????? ?????????????????????????????? ????????????\n\n"
                "2.???????????? ???????????????????????? ?????????????????? ??????????????? ???????????????????????????\n\n"
                "??????Java ????????????"));
        progressDialog->setRange(0, compressedMetatiles.count());

        //?????????????????????metatile??????
        int layerType = this->ui->comboBox_layerType->currentIndex();
        int encounterType = this->ui->comboBox_encounterType->currentIndex();
        int terrainType = this->ui->comboBox_terrainType->currentIndex();
        int behavior = this->ui->comboBox_metatileBehaviors->currentIndex();
        auto *selectedMetatile = new Metatile;
        memccpy(selectedMetatile,
                Tileset::getMetatile(selectedMetatileId, this->primaryTileset, this->secondaryTileset), 1,
                sizeof Metatile());

        //??????????????????????????????metatileId
        bool sequenceFixedFlag[sequence.count()];
        for (int i = 0; i < sequence.count(); i++)
            sequenceFixedFlag[i] = false;

        //????????????
        int nowY = 0;
        int offset = 0;
        if (right != 0)
        {
            nowY += right;
            selectedMetatileId += right;
        }
        //???????????????????????????
        for (int nowIndex = 0; nowIndex < compressedMetatiles.count(); nowIndex++)
        {
            QApplication::processEvents();
            //??????????????????????????????
            if(!allowEmpty)
            {
                if(isGBAEmptyImage(compressedMetatiles.at(nowIndex)))
                {
                    offset--;
                    continue;
                }
            }

            //??????????????????????????????????????????
            int nowProcess = selectedMetatileId + nowIndex + offset;
            if (++nowY == exceptWidth)
            {
                nowY = 0;
                offset = offset + 8 - exceptWidth;
            }

            //??????json????????????????????????
            //???????????????????????????
            for (int jsonArrIndex = 0; jsonArrIndex < sequence.count(); jsonArrIndex++)
            {
                //??????????????????????????????index??????
                if (sequence.at(jsonArrIndex).toInt() == nowIndex && !sequenceFixedFlag[jsonArrIndex])
                {
                    //???????????????
                    sequenceFixedFlag[jsonArrIndex] = true;
                    //??????????????????metatileId
                    sequence.replace(jsonArrIndex, nowProcess);
                }
            }

            progressDialog->setValue(nowIndex);
            Metatile *metatileToEdit = Tileset::getMetatile(nowProcess, this->primaryTileset,
                                                            this->secondaryTileset);
            metatileToEdit->layerType = layerType;
            metatileToEdit->encounterType = encounterType;
            metatileToEdit->terrainType = terrainType;
            metatileToEdit->behavior = behavior;
            metatileToEdit->label = "";
//            logInfo(QString("nowIndex %1 totalTiles %2 image %3x%4 %5x%6")
//                            .arg(nowIndex)
//                            .arg(totalTiles)
//                            .arg(image.width())
//                            .arg(image.height())
//                            .arg(numTilesWide)
//                            .arg(numTilesHigh)
//            );
            auto list = matchTile(compressedMetatiles.at(nowIndex));
            if (list.length() == 0)
            {
                progressDialog->close();
                msgBox.setInformativeText(tr("????????????\n"
                                             "1.???????????????(tile)????????? ????????????????????????\n"
                                             "2.?????????????????????\n"
                                             "??????????????????"));
                msgBox.exec();
                return;
            }
            if (isTripleLayerMetatile)
                for (int i = 0; i < 8; i++)
                    metatileToEdit->tiles.replace(i, selectedMetatile->tiles.at(i));
            else
                for (int i = 0; i < 4; i++)
                    metatileToEdit->tiles.replace(i, selectedMetatile->tiles.at(i));

            for (int i = 0; i < 4; i++)
            {
                metatileToEdit->tiles.replace(topTiles[i], list.at(i));
            }

        }
        //??????????????????????????? ?????????????????????????????????metatileId flag???false ????????????-1
        if(!allowEmpty)
        {
            for (int i = 0; i < sequence.count(); ++i)
            {
                if(!sequenceFixedFlag[i])
                {
                    sequence.replace(i,-1);
                }
            }
        }
        //????????????
        progressDialog->close();
    }
    else
        return;

    //??????json
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.setInformativeText(tr("?????????????????? %1 ??? ????????????????????? %2 ???\n\n"
                                      "??????????????????????????????????????????????????? ????????????????????? ?????????????????????\n\n")
                                      .arg(totalTiles).arg(compressedMetatiles.count()));
    msgBox.exec();
    msgBox.setIcon(QMessageBox::Icon::Critical);

    QString defaultPath = QString("%1.json").arg(filepath);
    QString jsonFilePath = QFileDialog::getSaveFileName(this,
                                                        tr("??????????????????json"),
                                                        defaultPath,
                                                        tr("??????json (*.json)"));

    QJsonObject jsonObject;
    jsonObject["sequence"] = sequence;
    jsonObject["width"] = numTilesWide;
    jsonObject["height"] = numTilesHigh;
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    QFile *jsonFile;
    if (!jsonFilePath.isEmpty())
        jsonFile = new QFile(jsonFilePath);
    else
        jsonFile = new QFile(defaultPath);

    if (!jsonFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        msgBox.setInformativeText(tr("?????????????????????????????????"));
        msgBox.exec();
        return;
    }
    jsonFile->write(jsonDoc.toJson());
    jsonFile->close();

    msgBox.setText(tr("??????"));
    msgBox.setInformativeText(tr("????????????????????????"));
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.exec();
    this->hasUnsavedChanges = true;
    this->refresh();
}

QList<Tile> TilesetEditor::matchTile(const QImage &image)
{
    int currentPaletteId = this->ui->spinBox_paletteSelector->value();
    QList<Tile> ret = QList<Tile>();
//    logInfo(QString("xOrigin %1 yOrigin %2")
//                .arg(xOrigin)
//            .arg(yOrigin)
//                );

    //??????metatile????????????tile
    int offset[4][2] = {{0, 0},
                        {8, 0},
                        {0, 8},
                        {8, 8}};

    //?????????metatile?????????
    for (auto &offsetIdx: offset)
    {
        //?????????????????????????????????????????????
        int x = offsetIdx[0];
        int y = offsetIdx[1];

        //??????
        QImage imageSlice = image.copy(x, y, 8, 8);

        //??????????????????tile ???????????????tile???slice
        bool matched = true;
        for (int i = 0;
             i < this->primaryTileset->tiles.count() + this->secondaryTileset->tiles.count();
             i++)
        {
//            logInfo(QString("now comparring %1")
//                            .arg(i)
//            );
            QImage tile = getPalettedTileImage(i, this->primaryTileset, this->secondaryTileset, currentPaletteId, true);

            auto result = matchImage(imageSlice, tile, true);
            if (result == MatchResult::NOT_MATCH)
            {
                matched = false;
                continue;
            }
            else if (result == MatchResult::MATCH)
            {
                matched = true;
                ret.append(Tile(i, false, false, paletteId));
                break;
            }
            else if (result == MatchResult::X_FLIP)
            {
                matched = true;
                ret.append(Tile(i, true, false, paletteId));
                break;
            }
            else if (result == MatchResult::Y_FLIP)
            {
                matched = true;
                ret.append(Tile(i, false, true, paletteId));
                break;
            }
            else if (result == MatchResult::XY_FLIP)
            {
                matched = true;
                ret.append(Tile(i, true, true, paletteId));
                break;
            }
        }
        //??????????????????tile??????????????????????????????
        if (!matched)
        {
            return {};
        }
    }
    if (ret.length() == 4)
    {
        return ret;
    }
    else
    {
        return {};
    }
}


void TilesetEditor::on_actionExpandPrimaryTile_triggered()
{
    expandTiles(true);
}


void TilesetEditor::on_actionExpandSecondaryTile_triggered()
{
    expandTiles(false);
}

void TilesetEditor::expandTiles(bool isPrimary)
{
    int count, max;
    if (isPrimary)
    {
        count = this->primaryTileset->tiles.count();
        max = Project::getNumTilesPrimary();
    }
    else
    {
        count = this->secondaryTileset->tiles.count();
        max = Project::getNumTilesTotal() - Project::getNumTilesPrimary();
    }

    QMessageBox msgBox(this);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Critical);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(tr("??????Tiles"));
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);
    auto *QSBHeight = new QSpinBox();
    QSBHeight->setMinimum(count);
    QSBHeight->setMaximum(max);
    form.addRow(new QLabel(tr("%1 Tiles ?????? %2 ?????????????????????????????????????????????????????????????????????????????? %3 ?????????16?????????")
                                   .arg(isPrimary ? "Primary" : "Secondary")
                                   .arg(count)
                                   .arg(max)));
    form.addRow(new QLabel(tr("???????????????")), QSBHeight);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted)
    {
        int expandTo = QSBHeight->value();
        if (expandTo % 16 != 0 || expandTo > max || expandTo <= count)
        {
            msgBox.setText(tr("??????"));
            msgBox.setInformativeText(tr("???????????????????????????????????????????????? %1 ?????????16?????????")
                                              .arg(max));
            msgBox.exec();
            return;
        }

        if (isPrimary)
        {
            QImage lastTile = this->primaryTileset->tiles.at(this->primaryTileset->tiles.count() - 1);
            for (int i = 0; i < expandTo - count; i++)
            {
                this->primaryTileset->tiles.append(lastTile);
            }
            this->primaryTileset->tilesImage = this->tileSelector->buildPrimaryTilesIndexedImage();
        }
        else
        {
            QImage lastTile = this->secondaryTileset->tiles.at(this->secondaryTileset->tiles.count() - 1);
            for (int i = 0; i < expandTo - count; i++)
            {
                this->secondaryTileset->tiles.append(lastTile);
            }
            this->secondaryTileset->tilesImage = this->tileSelector->buildSecondaryTilesIndexedImage();
        }
        this->refresh();
        this->hasUnsavedChanges = true;
    }
}


void TilesetEditor::on_actionExportPrimaryMetatilesToAM_triggered()
{
    exportMetatilesToAM(true);
}


void TilesetEditor::on_actionExportSecondaryMetatilesToAM_triggered()
{
    exportMetatilesToAM(false);
}


void TilesetEditor::exportMetatilesToAM(bool isPrimary)
{
    //?????????
    QMessageBox msgBox(this);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setText(tr("??????"));

    //?????????
    if (this->hasUnsavedChanges)
    {
        msgBox.setInformativeText(tr("??????????????????????????????????????????"));
        msgBox.exec();
        return;
    }

    //????????????
    QString filepath = QFileDialog::getSaveFileName(this,
                                                    tr("??????AM???????????????"),
                                                    this->project->root,
                                                    tr("AM??????????????? (*)"));
    if (filepath.isEmpty())
        return;

    //bvd??????
    auto bvdFile = new QFile(filepath + ".bvd");
    if (!bvdFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        msgBox.setInformativeText(tr("bvd??????????????????"));
        msgBox.exec();
        return;
    }

    //????????????t0t1????????????
    int metatilesCount = 0;
    Tileset *tileset;

    if (isPrimary)
    {
        tileset = this->primaryTileset;
    }
    else
    {
        tileset = this->secondaryTileset;
    }
    metatilesCount = tileset->metatiles.count();

    //???????????????
    byte b[4];
    b[0] = (byte) (metatilesCount & 0xff);
    b[1] = (byte) (metatilesCount >> 8 & 0xff);
    b[2] = (byte) (metatilesCount >> 16 & 0xff);
    b[3] = (byte) (metatilesCount >> 24 & 0xff);

    //???????????????
    QByteArray data;

    //??????
    data.append(static_cast<char>(b[0]));
    data.append(static_cast<char>(b[1]));
    data.append(static_cast<char>(b[2]));
    data.append(static_cast<char>(b[3]));

    //metatile????????????
    int numTiles = projectConfig.getTripleLayerMetatilesEnabled() ? 12 : 8;
    for (auto each: tileset->metatiles)
    {
        for (int i = 0; i < numTiles; i++)
        {
            uint16_t tile = each->tiles.at(i).rawValue();
            data.append(static_cast<char>(tile));
            data.append(static_cast<char>(tile >> 8));
        }
    }

    //tiles??????
    BaseGameVersion version = projectConfig.getBaseGameVersion();
    int attrSize = Metatile::getAttributesSize(version);
    for (auto each: tileset->metatiles)
    {
        uint32_t attributes = each->getAttributes(version);
        for (int i = 0; i < attrSize; i++)
            data.append(static_cast<char>(attributes >> (8 * i)));
    }

    //???????????????
    switch (projectConfig.getBaseGameVersion())
    {
        case pokeruby:
        case pokeemerald:
            data.append(static_cast<char>('\x52'));
            data.append(static_cast<char>('\x53'));
            data.append(static_cast<char>('\x45'));
            data.append(static_cast<char>('\x20'));
            break;
        default:
        case pokefirered:
            data.append(static_cast<char>('\x46'));
            data.append(static_cast<char>('\x52'));
            data.append(static_cast<char>('\x4c'));
            data.append(static_cast<char>('\x47'));
            break;
    }

    //??????
    bvdFile->write(data);
    bvdFile->close();

    //????????????
    data.clear();
    auto palFile = new QFile(filepath + ".pal");
    if (!palFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        msgBox.setInformativeText(tr("???????????????????????????"));
        msgBox.exec();
    }
    QList<QList<QRgb>> targetPal;
    if(isPrimary)
        targetPal = this->primaryTileset->palettes;
    else
        targetPal = this->secondaryTileset->palettes;
    for (auto each: targetPal.at(this->paletteId))
    {
        QColor color = QColor(each);
        data.append(static_cast<char>(color.red()));
        data.append(static_cast<char>(color.green()));
        data.append(static_cast<char>(color.blue()));
        data.append(static_cast<char>('\x00'));
    }
    palFile->write(data);
    palFile->close();

    //dib??????
    auto dibFile = new QFile(filepath + ".dib");
    if (!dibFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        msgBox.setInformativeText(tr("????????????????????????"));
        msgBox.exec();
    }
    QImage image;
    if(isPrimary)
        image = this->tileSelector->buildPrimaryTilesIndexedImage();
    else
        image = this->tileSelector->buildSecondaryTilesIndexedImage();

    image = image.convertToFormat(QImage::Format_RGB888);
    image.save(dibFile, "BMP");
    dibFile->close();

    //??????
    msgBox.setText(tr("??????"));
    msgBox.setInformativeText(tr("????????????"));
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.exec();

}

void TilesetEditor::on_actionExportCurrentPalttle_triggered()
{
    //?????????
    QMessageBox msgBox(this);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setText(tr("??????"));

    //????????????
    QString filepath = QFileDialog::getSaveFileName(this,
                                                    tr("?????????????????????"),
                                                    this->project->root,
                                                    tr("??????????????? (*.pal)"));
    if (filepath.isEmpty())
        return;
    auto palFile = new QFile(filepath);
    if (!palFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        msgBox.setInformativeText(tr("???????????????????????????"));
        msgBox.exec();
    }

    //????????????
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.setText(tr("??????"));
    msgBox.setInformativeText(tr("????????????????????????????????????????????????????????????tile?????????????????????"));
    msgBox.exec();
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setText(tr("??????"));

    //?????????????????????tile
    int selectedTileId = this->tileSelector->getSelectedTiles().at(0).tileId;
    bool primary = selectedTileId < this->primaryTileset->tiles.count();
    auto target = primary?this->primaryTileset->palettes:this->secondaryTileset->palettes;

    //???????????????
    QByteArray data;
    for (auto each: target.at(this->paletteId))
    {
        QColor color = QColor(each);
        data.append(static_cast<char>(color.red()));
        data.append(static_cast<char>(color.green()));
        data.append(static_cast<char>(color.blue()));
        data.append(static_cast<char>('\x00'));
    }
    palFile->write(data);
    palFile->close();

    //??????
    msgBox.setText(tr("??????"));
    msgBox.setInformativeText(tr("????????????"));
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.exec();
}

QList<QImage> TilesetEditor::compressImage(const QImage &image, bool allowFlip, int widthPerBlock, int heightPerBlock, /*NULLABLE*/QJsonArray *outputSequence)
{
    //????????????????????????
    int numTilesWide = image.width() / widthPerBlock;
    int numTilesHigh = image.height() / heightPerBlock;
    int totalTiles = numTilesHigh * numTilesWide;

    //?????????????????????
    auto *compressProgress = new QProgressDialog(this);
    compressProgress->setWindowModality(Qt::NonModal);
    compressProgress->setMinimumDuration(0);
    compressProgress->setCancelButton(nullptr);
    compressProgress->setWindowTitle(tr("?????????"));
    compressProgress->setLabelText(
            tr("1.??????????????? ??????????????? ?????????????????????????????? ????????????\n\n"
            "2.???????????? ???????????????????????? ?????????????????? ??????????????? ???????????????????????????\n\n"
            "??????Java ????????????"));
    compressProgress->setRange(0, totalTiles);
    //??????
    QList<QImage> compressedImage;
    //????????????????????????
    QJsonArray sequence;
    int nowBlock = 0;
    //??????????????????block
    compressedImage.append(image.copy(0, 0, widthPerBlock, heightPerBlock));
    bool matched;
    //????????????
    for (int y = 0; y < image.height(); y += heightPerBlock)
        for (int x = 0; x < image.width(); x += widthPerBlock)
        {
            compressProgress->setValue(x + y * image.height());
            //????????????
            QImage tile = image.copy(x, y, widthPerBlock, heightPerBlock);
            matched = false;
            //???????????????????????????
            for (int i = 0; i < compressedImage.count(); i++)
            {
                QApplication::processEvents();
                //????????????
                if (matchImage(tile, compressedImage.at(i), allowFlip) != MatchResult::NOT_MATCH)
                {
                    //????????????????????? ??????????????????????????? ??????????????????
                    sequence.append(i);
                    matched = true;
                    break;
                }
            }
            //????????????????????????
            if (!matched)
            {
                sequence.append(++nowBlock);
                compressedImage.append(tile);
            }
        }
    compressProgress->close();
    if (outputSequence != nullptr)
    {
        for(auto each:sequence)
        {
            outputSequence->append(each);
        }
    }
    return compressedImage;
}

TilesetEditor::DialogResult TilesetEditor::createParametersDialog(bool isMetatile, int selectedId, const QList<QImage>& compressedImageList)
{
    int lineMax;
    if (isMetatile)
        lineMax = 8;
    else
        lineMax = 16;
    //????????????
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(tr("??????"));
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);
    auto *QSBWidth = new QSpinBox();
    QSBWidth->setMinimum(1);
    QSBWidth->setMaximum(lineMax - (selectedId % lineMax));
    auto *QLBHeight = new QLabel();
    QLBHeight->setEnabled(false);
    QLBHeight->setText(QString("%1").arg((int) ceil(compressedImageList.count() * 1.0 / 1)));
    auto *QSBRight = new QSpinBox();
    QSBRight->setMinimum(0);
    QSBRight->setMaximum(QSBWidth->value()-1);
    auto *QCBAllowEmpty = new QCheckBox();
    QCBAllowEmpty->setChecked(true);
    form.addRow(new QLabel(tr("??????????????????????????????????????????????????? %1 ???tile?????????????????????").arg(compressedImageList.count())));
    form.addRow(new QLabel(tr("????????????")), QSBWidth);
    form.addRow(new QLabel(tr("???????????????")), QLBHeight);
    form.addRow(new QLabel(tr("??????x???????????? (0??????????????????)")), QSBRight);
    form.addRow(new QLabel(tr("????????????????????????")), QCBAllowEmpty);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(QSBWidth, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
        QSBRight->setMaximum(value-1);
        if (selectedId % lineMax + value > lineMax)
            QLBHeight->setText(QString(tr("????????????")));
        else
            QLBHeight->setText(QString("%1").arg((int) ceil(compressedImageList.count() * 1.0 / value)));
    });

    //???????????????
    if (dialog.exec() == QDialog::Accepted)
    {
        DialogResult result = DialogResult();
        result.right = QSBRight->value();
        result.width = QSBWidth->value();
        result.allowEmpty = QCBAllowEmpty->isChecked();
        result.isAccept = true;
        return result;
    }
    else
    {
        return {};
    }
}

bool TilesetEditor::isGBAEmptyImage(const QImage& image)
{
    int width = image.width();
    int height = image.height();

    bool success = true;
    for (int yCompIdx = 0; yCompIdx < height; yCompIdx++)
    {
        for (int xCompIdx = 0; xCompIdx < width; xCompIdx++)
        {
            if (image.pixelColor(xCompIdx, yCompIdx) !=
                image.colorTable().at(0))
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }

    return success;
}

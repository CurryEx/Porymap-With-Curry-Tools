#include "config.h"
#include "imageproviders.h"
#include "log.h"
#include <QPainter>

QImage getCollisionMetatileImage(Block block) {
    return getCollisionMetatileImage(block.collision, block.elevation);
}

QImage getCollisionMetatileImage(int collision, int elevation) {
    int x = collision * 16;
    int y = elevation * 16;
    QPixmap collisionImage = QPixmap(":/images/collisions.png").copy(x, y, 16, 16);
    return collisionImage.toImage();
}

QImage getMetatileImage(
        uint16_t metatileId,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        QList<int> layerOrder,
        QList<float> layerOpacity,
        bool useTruePalettes)
{
    Metatile* metatile = Tileset::getMetatile(metatileId, primaryTileset, secondaryTileset);
    Tileset* blockTileset = Tileset::getMetatileTileset(metatileId, primaryTileset, secondaryTileset);
    if (!metatile || !blockTileset) {
        QImage metatile_image(16, 16, QImage::Format_RGBA8888);
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }
    return getMetatileImage(metatile, primaryTileset, secondaryTileset, layerOrder, layerOpacity, useTruePalettes);
}

QImage getMetatileImage(
        Metatile *metatile,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        QList<int> layerOrder,
        QList<float> layerOpacity,
        bool useTruePalettes)
{
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);
    if (!metatile) {
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }
    metatile_image.fill(Qt::black);

    QList<QList<QRgb>> palettes = Tileset::getBlockPalettes(primaryTileset, secondaryTileset, useTruePalettes);

    QPainter metatile_painter(&metatile_image);
    bool isTripleLayerMetatile = projectConfig.getTripleLayerMetatilesEnabled();
    int numLayers = isTripleLayerMetatile ? 3: 2;
    for (int layer = 0; layer < numLayers; layer++)
    for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
        int l = layerOrder.size() >= numLayers ? layerOrder[layer] : layer;
        int bottomLayer = layerOrder.size() >= numLayers ? layerOrder[0] : 0;
        Tile tile = metatile->tiles.value((y * 2) + x + (l * 4));
        QImage tile_image = getTileImage(tile.tileId, primaryTileset, secondaryTileset);
        if (tile_image.isNull()) {
            // Some metatiles specify tiles that are outside the valid range.
            // These are treated as completely transparent, so they can be skipped without
            // being drawn unless they're on the bottom layer, in which case we need
            // a placeholder because garbage will be drawn otherwise.
            if (l == bottomLayer) {
                metatile_painter.fillRect(x * 8, y * 8, 8, 8, palettes.value(0).value(0));
            }
            continue;
        }

        // Colorize the metatile tiles with its palette.
        if (tile.palette < palettes.length()) {
            QList<QRgb> palette = palettes.value(tile.palette);
            for (int j = 0; j < palette.length(); j++) {
                tile_image.setColor(j, palette.value(j));
            }
        } else {
            logWarn(QString("Tile '%1' is referring to invalid palette number: '%2'").arg(tile.tileId).arg(tile.palette));
        }

        QPoint origin = QPoint(x*8, y*8);
        float opacity = layerOpacity.size() >= numLayers ? layerOpacity[l] : 1.0;
        if (opacity < 1.0) {
            int alpha = 255 * opacity;
            for (int c = 0; c < tile_image.colorCount(); c++) {
                QColor color(tile_image.color(c));
                color.setAlpha(alpha);
                tile_image.setColor(c, color.rgba());
            }
        }

        // The top layer of the metatile has its first color displayed at transparent.
        if (l != bottomLayer) {
            QColor color(tile_image.color(0));
            color.setAlpha(0);
            tile_image.setColor(0, color.rgba());
        }

        metatile_painter.drawImage(origin, tile_image.mirrored(tile.xflip, tile.yflip));
    }
    metatile_painter.end();

    return metatile_image;
}

QImage getTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getTileTileset(tileId, primaryTileset, secondaryTileset);
    int index = Tile::getIndexInTileset(tileId);
    if (!tileset) {
        return QImage();
    }
    return tileset->tiles.value(index, QImage());
}

QImage getColoredTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset, QList<QRgb> palette) {
    QImage tileImage = getTileImage(tileId, primaryTileset, secondaryTileset);
    if (tileImage.isNull()) {
        tileImage = QImage(8, 8, QImage::Format_RGBA8888);
        QPainter painter(&tileImage);
        painter.fillRect(0, 0, 8, 8, palette.at(0));
    } else {
        for (int i = 0; i < 16; i++) {
            tileImage.setColor(i, palette.at(i));
        }
    }

    return tileImage;
}

QImage getPalettedTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset, int paletteId, bool useTruePalettes) {
    QList<QRgb> palette = Tileset::getPalette(paletteId, primaryTileset, secondaryTileset, useTruePalettes);
    return getColoredTileImage(tileId, primaryTileset, secondaryTileset, palette);
}

QImage getGreyscaleTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return getColoredTileImage(tileId, primaryTileset, secondaryTileset, greyscalePalette);
}

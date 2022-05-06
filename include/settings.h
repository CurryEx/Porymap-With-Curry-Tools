#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCursor>
#include <QTranslator>

class Settings
{
public:
    Settings();
    bool smartPathsEnabled;
    bool betterCursors;
    QCursor mapCursor;
    bool playerViewRectEnabled;
    bool cursorTileRectEnabled;

    enum Language{
        Chinese,
        English
    };

    static Language language;
    static QTranslator * translator;
};

#endif // SETTINGS_H

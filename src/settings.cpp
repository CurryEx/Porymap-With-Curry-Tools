#include "settings.h"

Settings::Settings()
{
    this->smartPathsEnabled = false;
    this->betterCursors = true;
    this->playerViewRectEnabled = false;
    this->cursorTileRectEnabled = true;
}

QTranslator * Settings::translator = nullptr;
Settings::Language Settings::language = Settings::Chinese;

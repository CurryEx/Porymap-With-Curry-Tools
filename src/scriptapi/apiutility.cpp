#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "config.h"

ScriptUtility::ScriptUtility(MainWindow *mainWindow) {
    this->window = mainWindow;
}

void ScriptUtility::registerAction(QString functionName, QString actionName, QString shortcut) {
    if (!window || !window->ui || !window->ui->menuTools)
        return;

    this->actionMap.insert(actionName, functionName);
    if (this->actionMap.size() == 1) {
        QAction *section = window->ui->menuTools->addSection("Custom Actions");
        this->registeredActions.append(section);
    }
    QAction *action = window->ui->menuTools->addAction(actionName, [actionName](){
       Scripting::invokeAction(actionName);
    });
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    this->registeredActions.append(action);
}

void ScriptUtility::clearActions() {
    for (auto action : this->registeredActions) {
        window->ui->menuTools->removeAction(action);
    }
}

QString ScriptUtility::getActionFunctionName(QString actionName) {
    return this->actionMap.value(actionName);
}

void ScriptUtility::setTimeout(QJSValue callback, int milliseconds) {
  if (!callback.isCallable() || milliseconds < 0)
      return;

    QTimer *timer = new QTimer(0);
    connect(timer, &QTimer::timeout, [=](){
        this->callTimeoutFunction(callback);
    });
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->setSingleShot(true);
    timer->start(milliseconds);
}

void ScriptUtility::callTimeoutFunction(QJSValue callback) {
    Scripting::tryErrorJS(callback.call());
}

void ScriptUtility::log(QString message) {
    logInfo(message);
}

void ScriptUtility::warn(QString message) {
    logWarn(message);
}

void ScriptUtility::error(QString message) {
    logError(message);
}

void ScriptUtility::runMessageBox(QString text, QString informativeText, QString detailedText, QMessageBox::Icon icon) {
    QMessageBox messageBox(window);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(icon);
    messageBox.exec();
}

void ScriptUtility::showMessage(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Information);
}

void ScriptUtility::showWarning(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Warning);
}

void ScriptUtility::showError(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Critical);
}

bool ScriptUtility::showQuestion(QString text, QString informativeText, QString detailedText) {
    QMessageBox messageBox(window);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    return messageBox.exec() == QMessageBox::Yes;
}

QJSValue ScriptUtility::getInputText(QString title, QString label, QString defaultValue) {
    bool ok;
    QString input = QInputDialog::getText(window, title, label, QLineEdit::Normal, defaultValue, &ok);
    return Scripting::dialogInput(input, ok);
}

QJSValue ScriptUtility::getInputNumber(QString title, QString label, double defaultValue, double min, double max, int decimals, double step) {
    bool ok;
    double input = QInputDialog::getDouble(window, title, label, defaultValue, min, max, decimals, &ok, Qt::WindowFlags(), step);
    return Scripting::dialogInput(input, ok);
}

QJSValue ScriptUtility::getInputItem(QString title, QString label, QStringList items, int defaultValue, bool editable) {
    bool ok;
    QString input = QInputDialog::getItem(window, title, label, items, defaultValue, editable, &ok);
    return Scripting::dialogInput(input, ok);
}

int ScriptUtility::getMainTab() {
    if (!window || !window->ui || !window->ui->mainTabBar)
        return -1;
    return window->ui->mainTabBar->currentIndex();
}

void ScriptUtility::setMainTab(int index) {
    if (!window || !window->ui || !window->ui->mainTabBar || index < 0 || index >= window->ui->mainTabBar->count())
        return;
    // Can't select Wild Encounters tab if it's disabled
    if (index == 4 && !userConfig.getEncounterJsonActive())
        return;
    window->on_mainTabBar_tabBarClicked(index);
}

int ScriptUtility::getMapViewTab() {
    if (!window || !window->ui || !window->ui->mapViewTab)
        return -1;
    return window->ui->mapViewTab->currentIndex();
}

void ScriptUtility::setMapViewTab(int index) {
    if (this->getMainTab() != 0 || !window->ui->mapViewTab || index < 0 || index >= window->ui->mapViewTab->count())
        return;
    window->on_mapViewTab_tabBarClicked(index);
}

void ScriptUtility::setGridVisibility(bool visible) {
    window->ui->checkBox_ToggleGrid->setChecked(visible);
}

bool ScriptUtility::getGridVisibility() {
    return window->ui->checkBox_ToggleGrid->isChecked();
}

void ScriptUtility::setBorderVisibility(bool visible) {
    window->editor->toggleBorderVisibility(visible, false);
}

bool ScriptUtility::getBorderVisibility() {
    return window->ui->checkBox_ToggleBorder->isChecked();
}

void ScriptUtility::setSmartPathsEnabled(bool visible) {
    window->ui->checkBox_smartPaths->setChecked(visible);
}

bool ScriptUtility::getSmartPathsEnabled() {
    return window->ui->checkBox_smartPaths->isChecked();
}

QList<QString> ScriptUtility::getCustomScripts() {
    return userConfig.getCustomScripts();
}

QList<int> ScriptUtility::getMetatileLayerOrder() {
    if (!window || !window->editor || !window->editor->map)
        return QList<int>();
    return window->editor->map->metatileLayerOrder;
}

void ScriptUtility::setMetatileLayerOrder(QList<int> order) {
    if (!window || !window->editor || !window->editor->map)
        return;

    const int numLayers = 3;
    int size = order.size();
    if (size < numLayers) {
        logError(QString("Metatile layer order has insufficient elements (%1), needs at least %2.").arg(size).arg(numLayers));
        return;
    }
    bool invalid = false;
    for (int i = 0; i < numLayers; i++) {
        int layer = order.at(i);
        if (layer < 0 || layer >= numLayers) {
            logError(QString("'%1' is not a valid metatile layer order value, must be in range 0-%2.").arg(layer).arg(numLayers - 1));
            invalid = true;
        }
    }
    if (invalid) return;

    window->editor->map->metatileLayerOrder = order;
    window->refreshAfterPalettePreviewChange();
}

QList<float> ScriptUtility::getMetatileLayerOpacity() {
    if (!window || !window->editor || !window->editor->map)
        return QList<float>();
    return window->editor->map->metatileLayerOpacity;
}

void ScriptUtility::setMetatileLayerOpacity(QList<float> order) {
    if (!window || !window->editor || !window->editor->map)
        return;
    window->editor->map->metatileLayerOpacity = order;
    window->refreshAfterPalettePreviewChange();
}

QList<QString> ScriptUtility::getMapNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapNames;
}

QList<QString> ScriptUtility::getTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->tilesetLabelsOrdered;
}

QList<QString> ScriptUtility::getPrimaryTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->tilesetLabels["primary"];
}

QList<QString> ScriptUtility::getSecondaryTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->tilesetLabels["secondary"];
}

QList<QString> ScriptUtility::getMetatileBehaviorNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->metatileBehaviorMap.keys();
}

QList<QString> ScriptUtility::getSongNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->songNames;
}

QList<QString> ScriptUtility::getLocationNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapSectionNameToValue.keys();
}

QList<QString> ScriptUtility::getWeatherNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->weatherNames;
}

QList<QString> ScriptUtility::getMapTypeNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapTypes;
}

QList<QString> ScriptUtility::getBattleSceneNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapBattleScenes;
}

bool ScriptUtility::isPrimaryTileset(QString tilesetName) {
    return getPrimaryTilesetNames().contains(tilesetName);
}

bool ScriptUtility::isSecondaryTileset(QString tilesetName) {
    return getSecondaryTilesetNames().contains(tilesetName);
}

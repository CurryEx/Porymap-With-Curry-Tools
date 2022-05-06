#include "mainwindow.h"
#include "log.h"
#include "settings.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    QApplication a(argc, argv);

    //读取翻译
    QTranslator *trans = new QTranslator;
    Settings::translator = trans;
    QLocale locale;
    if(locale.language()!=QLocale::Chinese)
    {
        trans->load(qApp->applicationDirPath() + QString("/en.qm"));
        Settings::language = Settings::English;
        a.installTranslator(trans);
    }

    a.setStyle("fusion");
    MainWindow w(nullptr);
    w.show();

    auto result = a.exec();
    if(trans!=nullptr)
        delete trans;
    return result;
}

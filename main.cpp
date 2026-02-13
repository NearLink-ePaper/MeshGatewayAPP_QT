#include "mainwindow.h"
#include "meshprotocol.h"
#include "stylemanager.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("NearLink Mesh");
    a.setOrganizationName("wellwhz");
    a.setApplicationVersion(APP_VERSION_STR);

    // 注册自定义类型，确保信号槽跨线程工作
    qRegisterMetaType<MeshNode>("MeshNode");
    qRegisterMetaType<UpstreamMessage>("UpstreamMessage");

    // 多语言支持 — 从 qrc 资源加载 .qm
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "NearLink_Mesh_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // iOS 风格暗色主题
    a.setStyleSheet(StyleManager::loadStyleSheet());

    // 全局默认字体
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    QFont defaultFont("SF Pro Display", 13);
#elif defined(Q_OS_WIN)
    QFont defaultFont("Segoe UI", 10);
#else
    QFont defaultFont("Noto Sans", 10);
#endif
    a.setFont(defaultFont);

    MainWindow w;
    w.show();
    return a.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "appcontroller.h"
#include "bootstrap.h"
#include "thememanager.h"
#include "nixadapter.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("YAS"));
    app.setApplicationName(QStringLiteral("yas-nix"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setApplicationDisplayName(QStringLiteral("Yet Another Store for Nix"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    yas::loadBundledFonts();

    NixAdapter adapter;
    yas::AppController controller(&adapter);
    yas::ThemeManager themeManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("YasManager"), &themeManager);
    engine.loadFromModule("YasNix", "Main");
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}

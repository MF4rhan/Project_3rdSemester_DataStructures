#include "MainWindow.h"

#include <QApplication>
#include <QGuiApplication>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("HuffmanProject"));
    app.setApplicationName(QStringLiteral("HuffmanStudio"));

    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion")))
    {
        app.setStyle(fusion);
    }

    MainWindow window;
    window.show();

    return app.exec();
}

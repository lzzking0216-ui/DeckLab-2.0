#include <QApplication>
#include <QFont>
#include <QDebug>
#include <cstdio>
#include "mainwindow.h"

// Debug 模式下：让 Qt Creator 的"应用程序输出"显示可点击的文件:行号
static void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    const char* level = "";
    switch (type) {
        case QtDebugMsg:    level = "DEBUG";    break;
        case QtInfoMsg:     level = "INFO";     break;
        case QtWarningMsg:  level = "WARNING";  break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg:    level = "FATAL";    break;
    }
    // Qt Creator 识别 "file(line): " 格式并渲染为超链接
    if (ctx.file && ctx.line > 0)
        fprintf(stderr, "%s(%d): [%s] %s\n", ctx.file, ctx.line, level, msg.toLocal8Bit().constData());
    else
        fprintf(stderr, "[%s] %s\n", level, msg.toLocal8Bit().constData());

    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char* argv[])
{
    qInstallMessageHandler(messageHandler);

    QApplication app(argc, argv);

    app.setApplicationName("DeckLab");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DeckLab");

    // 全局字体
    QFont font = app.font();
    font.setPixelSize(13);
    app.setFont(font);

    MainWindow w;
    w.show();

    return app.exec();
}

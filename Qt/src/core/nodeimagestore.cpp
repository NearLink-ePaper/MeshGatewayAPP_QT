#include "nodeimagestore.h"
#include <QDir>
#include <QStandardPaths>

QString NodeImageStore::getDir()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                   + QStringLiteral("/node_images");
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    return path;
}

void NodeImageStore::save(quint16 nodeAddr, const QImage &bitmap)
{
    QString file = getDir() + QStringLiteral("/node_%1.png")
                       .arg(nodeAddr, 4, 16, QChar('0')).toUpper();
    bitmap.save(file, "PNG");
}

QImage NodeImageStore::load(quint16 nodeAddr)
{
    QString file = getDir() + QStringLiteral("/node_%1.png")
                       .arg(nodeAddr, 4, 16, QChar('0')).toUpper();
    QImage img;
    img.load(file);
    return img;
}

QMap<quint16, QImage> NodeImageStore::loadAll()
{
    QMap<quint16, QImage> map;
    QDir dir(getDir());
    const QStringList files = dir.entryList({QStringLiteral("node_*.png")}, QDir::Files);
    for (const QString &fname : files) {
        QString base = fname.left(fname.lastIndexOf('.'));  // "node_0A01"
        if (!base.startsWith(QStringLiteral("node_")))
            continue;
        QString addrHex = base.mid(5);
        bool ok = false;
        quint16 addr = addrHex.toUShort(&ok, 16);
        if (!ok) continue;
        QImage img;
        if (img.load(dir.filePath(fname)))
            map[addr] = img;
    }
    return map;
}

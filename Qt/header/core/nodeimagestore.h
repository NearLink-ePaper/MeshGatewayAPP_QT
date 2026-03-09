#ifndef NODEIMAGESTORE_H
#define NODEIMAGESTORE_H

#include <QImage>
#include <QMap>
#include <QString>

/**
 * 节点图片持久化工具
 * 保存/加载每个节点最后一次发送的二值化图片
 */
class NodeImageStore
{
public:
    static void save(quint16 nodeAddr, const QImage &bitmap);
    static QImage load(quint16 nodeAddr);
    static QMap<quint16, QImage> loadAll();

private:
    static QString getDir();
};

#endif // NODEIMAGESTORE_H

#include "platform/win/icon_factory.h"

#include <QColor>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QPixmap>

namespace {

QIcon drawGlobeIcon()
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(40, 110, 200), 1.4));
    painter.setBrush(QColor(90, 155, 235));
    painter.drawEllipse(QRectF(1.5, 1.5, 13, 13));
    painter.setPen(QPen(QColor(255, 255, 255), 0.9));
    painter.drawEllipse(QRectF(5.5, 1.8, 5, 12.4));
    painter.drawLine(QPointF(1.8, 8), QPointF(14.2, 8));
    painter.end();
    return QIcon(pixmap);
}

QIcon drawTerminalIcon()
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(38, 44, 52));
    painter.drawRoundedRect(QRectF(1, 2, 14, 12), 2, 2);
    painter.setPen(QPen(QColor(120, 220, 140), 1.5));
    painter.drawLine(QPointF(4, 6), QPointF(6, 8));
    painter.drawLine(QPointF(6, 8), QPointF(4, 10));
    painter.drawLine(QPointF(8, 10.5), QPointF(12, 10.5));
    painter.end();
    return QIcon(pixmap);
}

} // namespace

QIcon IconFactory::forItem(const Item &item, const PathResolver &resolver)
{
    switch (item.type) {
    case ItemType::Directory:
    case ItemType::Application:
    case ItemType::File: {
        QFileIconProvider provider;
        const QFileInfo info(resolver.expand(item.path));
        const QIcon icon = provider.icon(info);
        return icon.isNull() ? provider.icon(QFileIconProvider::File) : icon;
    }
    case ItemType::Url: {
        if (!item.browser.programPath.isEmpty()) {
            const QFileInfo browserInfo(resolver.expand(item.browser.programPath));
            if (browserInfo.isFile()) {
                QFileIconProvider provider;
                const QIcon icon = provider.icon(browserInfo);
                if (!icon.isNull()) {
                    return icon;
                }
            }
        }
        return drawGlobeIcon();
    }
    case ItemType::Command:
        return drawTerminalIcon();
    }
    return QIcon();
}

QIcon IconFactory::applicationIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(46, 160, 92));
    painter.drawRoundedRect(QRectF(2, 2, 60, 60), 14, 14);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(38);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("W"));
    painter.end();
    return QIcon(pixmap);
}

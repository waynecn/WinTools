#include "ui/drag_widgets.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

GroupListWidget::GroupListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

void GroupListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->source() == this) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void GroupListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->source() == this) {
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void GroupListWidget::dropEvent(QDropEvent *event)
{
    if (event->source() == this) {
        const int from = currentRow();
        const QModelIndex index = indexAt(event->pos());
        const int to = index.isValid() ? index.row() : count() - 1;
        if (from >= 0 && to >= 0 && from != to) {
            emit rowsMoveRequested(from, to);
        }
        event->acceptProposedAction();
        return;
    }
    QListWidget::dropEvent(event);
}

ItemTableWidget::ItemTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

void ItemTableWidget::setExternalDropEnabled(bool enabled)
{
    m_externalDropEnabled = enabled;
}

void ItemTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        if (m_externalDropEnabled) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }
    if (event->source() == this) {
        event->acceptProposedAction();
        return;
    }
    QTableWidget::dragEnterEvent(event);
}

void ItemTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        if (m_externalDropEnabled) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }
    if (event->source() == this) {
        event->acceptProposedAction();
        return;
    }
    QTableWidget::dragMoveEvent(event);
}

void ItemTableWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        if (m_externalDropEnabled) {
            emit filesDropped(event->mimeData()->urls());
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }
    if (event->source() == this) {
        const int from = currentRow();
        const QModelIndex index = indexAt(event->pos());
        const int to = index.isValid() ? index.row() : rowCount() - 1;
        if (from >= 0 && to >= 0 && from != to) {
            emit rowsMoveRequested(from, to);
        }
        event->acceptProposedAction();
        return;
    }
    QTableWidget::dropEvent(event);
}

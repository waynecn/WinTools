#ifndef WINTOOLS_UI_DRAG_WIDGETS_H
#define WINTOOLS_UI_DRAG_WIDGETS_H

#include <QListWidget>
#include <QTableWidget>
#include <QUrl>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class GroupListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit GroupListWidget(QWidget *parent = nullptr);

signals:
    void rowsMoveRequested(int from, int to);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

class ItemTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit ItemTableWidget(QWidget *parent = nullptr);

    void setExternalDropEnabled(bool enabled);

signals:
    void rowsMoveRequested(int from, int to);
    void filesDropped(const QList<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool m_externalDropEnabled = true;
};

#endif // WINTOOLS_UI_DRAG_WIDGETS_H

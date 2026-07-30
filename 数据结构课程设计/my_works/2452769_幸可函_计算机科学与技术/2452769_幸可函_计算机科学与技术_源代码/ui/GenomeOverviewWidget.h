#ifndef GENOMEOVERVIEWWIDGET_H
#define GENOMEOVERVIEWWIDGET_H

#include <QRectF>
#include <QVector>
#include <QWidget>

#include <functional>

class SearchSessionModel;

class GenomeOverviewWidget : public QWidget
{
public:
    explicit GenomeOverviewWidget(QWidget *parent = nullptr);

    void SetSession(const SearchSessionModel *session);
    void SetViewportRange(int start, int end);
    void Refresh();
    void SetPositionActivatedCallback(
        const std::function<void(int)> &callback);
    void SetMatchActivatedCallback(
        const std::function<void(int, int)> &callback);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct HitRegion
    {
        QRectF rect;
        int readIndex;
        int matchIndex;
    };

    const SearchSessionModel *m_session;
    int m_viewStart;
    int m_viewEnd;
    QVector<HitRegion> m_hitRegions;
    std::function<void(int)> m_positionActivatedCallback;
    std::function<void(int, int)> m_matchActivatedCallback;

    QRectF contentRect() const;
    int positionAtX(qreal x) const;
};

#endif // GENOMEOVERVIEWWIDGET_H

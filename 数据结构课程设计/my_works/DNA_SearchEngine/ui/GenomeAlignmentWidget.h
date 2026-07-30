#ifndef GENOMEALIGNMENTWIDGET_H
#define GENOMEALIGNMENTWIDGET_H

#include <QAbstractScrollArea>
#include <QLineF>
#include <QPoint>
#include <QRectF>
#include <QVector>

#include <functional>

class SearchSessionModel;

class GenomeAlignmentWidget : public QAbstractScrollArea
{
public:
    explicit GenomeAlignmentWidget(QWidget *parent = nullptr);

    void SetSession(const SearchSessionModel *session);
    void Refresh();
    void RebuildLayout();
    void ClearLayout();
    void CenterOnPosition(int position);
    void FocusSelectedMatch();
    void FitWholeGenome();
    void ResetView();
    void SetZoomPercent(int percent);
    int ZoomPercent() const;
    int ViewportStart() const;
    int ViewportEnd() const;
    int LaneForMatch(int readIndex, int matchIndex) const;
    QRectF MatchViewportRect(int readIndex, int matchIndex) const;
    int LastPaintedConnectionCount() const;
    int LastPaintedConnectionSegmentCount() const;
    int LastPaintedSelectedMismatchCount() const;
    int LastPaintedReadLabelCount() const;
    void SetViewportChangedCallback(
        const std::function<void(int, int)> &callback);
    void SetMatchActivatedCallback(
        const std::function<void(int, int)> &callback);
    void SetSelectionClearedCallback(
        const std::function<void()> &callback);
    void SetZoomPercentChangedCallback(
        const std::function<void(int)> &callback);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct ReadLayoutItem
    {
        int readIndex;
        int matchIndex;
        int lane;
    };

    struct ReadHitRegion
    {
        QRectF rect;
        int readIndex;
        int matchIndex;
    };

    static constexpr int GutterWidth = 124;
    static constexpr int ReferenceTop = 58;
    static constexpr int TrackHeight = 30;
    static constexpr int ReadTop = 112;
    static constexpr int ReadLaneHeight = 44;

    const SearchSessionModel *m_session;
    int m_referenceLength;
    double m_pixelsPerBase;
    QVector<ReadLayoutItem> m_readLayout;
    QVector<ReadHitRegion> m_readHitRegions;
    int m_laneCount;
    int m_lastPaintedConnectionCount;
    int m_lastPaintedConnectionSegmentCount;
    int m_lastPaintedSelectedMismatchCount;
    int m_lastPaintedReadLabelCount;
    bool m_dragging;
    bool m_dragMoved;
    QPoint m_dragOrigin;
    int m_scrollOrigin;
    std::function<void(int, int)> m_viewportChangedCallback;
    std::function<void(int, int)> m_matchActivatedCallback;
    std::function<void()> m_selectionClearedCallback;
    std::function<void(int)> m_zoomPercentChangedCallback;

    void updateScrollRange();
    void updateVerticalScrollRange();
    int contentWidth() const;
    int genomePositionAt(qreal x) const;
    qreal xForPosition(int position) const;
    void emitViewportChanged();
    void setPixelsPerBase(double value, qreal anchorX = -1.0);
    const ReadLayoutItem *layoutItemFor(int readIndex,
                                        int matchIndex) const;
    QVector<QLineF> connectorSegments(
        int absolutePosition,
        const ReadLayoutItem &selected,
        qreal x,
        qreal startY,
        qreal endY) const;
    void showTooltipAt(const QPointF &point, const QPoint &globalPoint);
};

#endif // GENOMEALIGNMENTWIDGET_H

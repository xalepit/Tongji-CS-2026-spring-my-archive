#include "GenomeAlignmentWidget.h"

#include "SearchSessionModel.h"

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

bool hasMismatchAt(const MatchViewData &match, int offset)
{
    for (const MismatchViewData &mismatch : match.mismatches) {
        if (mismatch.offset == offset) {
            return true;
        }
    }
    return false;
}

} // namespace

GenomeAlignmentWidget::GenomeAlignmentWidget(QWidget *parent)
    : QAbstractScrollArea(parent),
      m_session(nullptr),
      m_referenceLength(0),
      m_pixelsPerBase(13.0),
      m_laneCount(0),
      m_lastPaintedConnectionCount(0),
      m_lastPaintedConnectionSegmentCount(0),
      m_lastPaintedSelectedMismatchCount(0),
      m_lastPaintedReadLabelCount(0),
      m_dragging(false),
      m_dragMoved(false),
      m_scrollOrigin(0)
{
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewport()->setMouseTracking(true);
    viewport()->setCursor(Qt::OpenHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(160);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        viewport()->update();
        emitViewportChanged();
    });
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            viewport(), qOverload<>(&QWidget::update));
}

void GenomeAlignmentWidget::SetSession(const SearchSessionModel *session)
{
    m_session = session;
    const int newLength =
        m_session == nullptr
            ? 0
            : static_cast<int>(m_session->Reference().size());
    if (newLength != m_referenceLength) {
        m_referenceLength = newLength;
        ClearLayout();
        horizontalScrollBar()->setValue(0);
    }
    updateScrollRange();
    viewport()->update();
    emitViewportChanged();
}

void GenomeAlignmentWidget::Refresh()
{
    updateScrollRange();
    viewport()->update();
}

void GenomeAlignmentWidget::RebuildLayout()
{
    m_readLayout.clear();
    m_laneCount = 0;
    m_readHitRegions.clear();
    if (m_session == nullptr) {
        updateVerticalScrollRange();
        viewport()->update();
        return;
    }

    struct Candidate
    {
        int readIndex;
        int matchIndex;
        int start;
        int end;
        int readId;
    };
    QVector<Candidate> candidates;
    for (int readIndex = 0;
         readIndex < m_session->Reads().size();
         ++readIndex) {
        const ReadResultViewData &read =
            m_session->Reads().at(readIndex);
        for (int matchIndex = 0;
             matchIndex < read.matches.size();
             ++matchIndex) {
            const MatchViewData &match = read.matches.at(matchIndex);
            candidates.append({readIndex,
                               matchIndex,
                               match.start,
                               match.End(),
                               read.readId});
        }
    }
    std::sort(candidates.begin(),
              candidates.end(),
              [](const Candidate &left, const Candidate &right) {
                  if (left.start != right.start) {
                      return left.start < right.start;
                  }
                  if (left.readId != right.readId) {
                      return left.readId < right.readId;
                  }
                  if (left.matchIndex != right.matchIndex) {
                      return left.matchIndex < right.matchIndex;
                  }
                  return left.readIndex < right.readIndex;
              });

    QVector<int> laneEnds;
    for (const Candidate &candidate : candidates) {
        int lane = 0;
        while (lane < laneEnds.size()
               && candidate.start < laneEnds.at(lane)) {
            ++lane;
        }
        if (lane == laneEnds.size()) {
            laneEnds.append(candidate.end);
        } else {
            laneEnds[lane] = candidate.end;
        }
        m_readLayout.append(
            {candidate.readIndex, candidate.matchIndex, lane});
    }
    m_laneCount = static_cast<int>(laneEnds.size());
    verticalScrollBar()->setValue(0);
    updateVerticalScrollRange();
    viewport()->update();
}

void GenomeAlignmentWidget::ClearLayout()
{
    m_readLayout.clear();
    m_readHitRegions.clear();
    m_laneCount = 0;
    m_lastPaintedConnectionCount = 0;
    m_lastPaintedSelectedMismatchCount = 0;
    verticalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setValue(0);
    viewport()->update();
}

void GenomeAlignmentWidget::CenterOnPosition(int position)
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return;
    }
    const int visiblePixels = std::max(1, viewport()->width() - GutterWidth);
    const int target =
        static_cast<int>(position * m_pixelsPerBase - visiblePixels / 2.0);
    horizontalScrollBar()->setValue(target);
}

void GenomeAlignmentWidget::FocusSelectedMatch()
{
    if (m_session == nullptr || m_session->SelectedMatch() == nullptr) {
        return;
    }
    const MatchViewData &match = *m_session->SelectedMatch();
    CenterOnPosition(match.start + match.length / 2);
}

void GenomeAlignmentWidget::FitWholeGenome()
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return;
    }
    const int available = std::max(1, viewport()->width() - GutterWidth - 8);
    setPixelsPerBase(
        std::max(
            0.18,
            static_cast<double>(available)
                / static_cast<double>(
                    m_session->Reference().size())));
    horizontalScrollBar()->setValue(0);
}

void GenomeAlignmentWidget::ResetView()
{
    setPixelsPerBase(13.0);
    FocusSelectedMatch();
}

void GenomeAlignmentWidget::SetZoomPercent(int percent)
{
    const int bounded = std::clamp(percent, 0, 100);
    const double value = 0.55 * std::pow(52.0, bounded / 100.0);
    setPixelsPerBase(value);
}

int GenomeAlignmentWidget::ZoomPercent() const
{
    const double normalized =
        std::log(std::max(0.55, m_pixelsPerBase) / 0.55) / std::log(52.0);
    return std::clamp(static_cast<int>(std::round(normalized * 100.0)),
                      0,
                      100);
}

int GenomeAlignmentWidget::ViewportStart() const
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return 0;
    }
    return std::clamp(
        static_cast<int>(horizontalScrollBar()->value() / m_pixelsPerBase),
        0,
        static_cast<int>(m_session->Reference().size()));
}

int GenomeAlignmentWidget::ViewportEnd() const
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return 0;
    }
    const int available = std::max(1, viewport()->width() - GutterWidth);
    return std::clamp(
        static_cast<int>(std::ceil(
            (horizontalScrollBar()->value() + available) / m_pixelsPerBase)),
        ViewportStart(),
        static_cast<int>(m_session->Reference().size()));
}

int GenomeAlignmentWidget::LaneForMatch(int readIndex, int matchIndex) const
{
    const ReadLayoutItem *item = layoutItemFor(readIndex, matchIndex);
    return item == nullptr ? -1 : item->lane;
}

QRectF GenomeAlignmentWidget::MatchViewportRect(int readIndex,
                                                int matchIndex) const
{
    const ReadLayoutItem *item = layoutItemFor(readIndex, matchIndex);
    if (item == nullptr || m_session == nullptr) {
        return QRectF();
    }
    const ReadResultViewData *read = m_session->ReadAt(readIndex);
    if (read == nullptr
        || matchIndex < 0
        || matchIndex >= read->matches.size()) {
        return QRectF();
    }
    const MatchViewData &match = read->matches.at(matchIndex);
    return QRectF(xForPosition(match.start),
                  ReadTop + item->lane * ReadLaneHeight
                      - verticalScrollBar()->value(),
                  match.length * m_pixelsPerBase,
                  TrackHeight);
}

int GenomeAlignmentWidget::LastPaintedConnectionCount() const
{
    return m_lastPaintedConnectionCount;
}

int GenomeAlignmentWidget::LastPaintedConnectionSegmentCount() const
{
    return m_lastPaintedConnectionSegmentCount;
}

int GenomeAlignmentWidget::LastPaintedSelectedMismatchCount() const
{
    return m_lastPaintedSelectedMismatchCount;
}

int GenomeAlignmentWidget::LastPaintedReadLabelCount() const
{
    return m_lastPaintedReadLabelCount;
}

void GenomeAlignmentWidget::SetViewportChangedCallback(
    const std::function<void(int, int)> &callback)
{
    m_viewportChangedCallback = callback;
}

void GenomeAlignmentWidget::SetMatchActivatedCallback(
    const std::function<void(int, int)> &callback)
{
    m_matchActivatedCallback = callback;
}

void GenomeAlignmentWidget::SetSelectionClearedCallback(
    const std::function<void()> &callback)
{
    m_selectionClearedCallback = callback;
}

void GenomeAlignmentWidget::SetZoomPercentChangedCallback(
    const std::function<void(int)> &callback)
{
    m_zoomPercentChangedCallback = callback;
}

void GenomeAlignmentWidget::updateScrollRange()
{
    const int available = std::max(1, viewport()->width() - GutterWidth);
    const int maximum = std::max(0, contentWidth() - available);
    horizontalScrollBar()->setRange(0, maximum);
    horizontalScrollBar()->setPageStep(available);
    horizontalScrollBar()->setSingleStep(
        std::max(1, static_cast<int>(m_pixelsPerBase * 10.0)));
    updateVerticalScrollRange();
}

void GenomeAlignmentWidget::updateVerticalScrollRange()
{
    const int available =
        std::max(1, viewport()->height() - ReadTop - 8);
    const int contentHeight = m_laneCount * ReadLaneHeight;
    verticalScrollBar()->setRange(
        0, std::max(0, contentHeight - available));
    verticalScrollBar()->setPageStep(available);
    verticalScrollBar()->setSingleStep(ReadLaneHeight);
}

int GenomeAlignmentWidget::contentWidth() const
{
    if (m_session == nullptr) {
        return 0;
    }
    return static_cast<int>(
        std::ceil(
            static_cast<double>(m_session->Reference().size())
            * m_pixelsPerBase));
}

int GenomeAlignmentWidget::genomePositionAt(qreal x) const
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return -1;
    }
    const qreal contentX =
        x - GutterWidth + horizontalScrollBar()->value();
    return std::clamp(static_cast<int>(std::floor(contentX / m_pixelsPerBase)),
                      0,
                      static_cast<int>(m_session->Reference().size()) - 1);
}

qreal GenomeAlignmentWidget::xForPosition(int position) const
{
    return GutterWidth + position * m_pixelsPerBase
        - horizontalScrollBar()->value();
}

void GenomeAlignmentWidget::emitViewportChanged()
{
    if (m_viewportChangedCallback) {
        m_viewportChangedCallback(ViewportStart(), ViewportEnd());
    }
}

void GenomeAlignmentWidget::setPixelsPerBase(double value, qreal anchorX)
{
    const double bounded = std::clamp(value, 0.2, 30.0);
    if (std::abs(bounded - m_pixelsPerBase) < 0.001) {
        return;
    }

    const qreal anchor =
        anchorX < GutterWidth ? viewport()->width() / 2.0 : anchorX;
    const double anchoredPosition =
        (horizontalScrollBar()->value() + anchor - GutterWidth)
        / m_pixelsPerBase;
    m_pixelsPerBase = bounded;
    updateScrollRange();
    const int newScroll = static_cast<int>(
        anchoredPosition * m_pixelsPerBase - anchor + GutterWidth);
    horizontalScrollBar()->setValue(newScroll);
    viewport()->update();
    if (m_zoomPercentChangedCallback) {
        m_zoomPercentChangedCallback(ZoomPercent());
    }
    emitViewportChanged();
}

const GenomeAlignmentWidget::ReadLayoutItem *
GenomeAlignmentWidget::layoutItemFor(int readIndex, int matchIndex) const
{
    for (const ReadLayoutItem &item : m_readLayout) {
        if (item.readIndex == readIndex
            && item.matchIndex == matchIndex) {
            return &item;
        }
    }
    return nullptr;
}

QVector<QLineF> GenomeAlignmentWidget::connectorSegments(
    int absolutePosition,
    const ReadLayoutItem &selected,
    qreal x,
    qreal startY,
    qreal endY) const
{
    QVector<QLineF> segments;
    if (m_session == nullptr || endY <= startY) {
        return segments;
    }

    QVector<QPair<qreal, qreal>> blockedRanges;
    for (const ReadLayoutItem &item : m_readLayout) {
        if (item.readIndex == selected.readIndex
            && item.matchIndex == selected.matchIndex) {
            continue;
        }
        const ReadResultViewData *read =
            m_session->ReadAt(item.readIndex);
        if (read == nullptr
            || item.matchIndex < 0
            || item.matchIndex >= read->matches.size()) {
            continue;
        }
        const MatchViewData &match = read->matches.at(item.matchIndex);
        if (absolutePosition < match.start
            || absolutePosition >= match.End()) {
            continue;
        }
        const int otherY = ReadTop + item.lane * ReadLaneHeight
            - verticalScrollBar()->value();
        const qreal blockedTop = std::max(startY,
                                          static_cast<qreal>(otherY));
        const qreal blockedBottom =
            std::min(endY,
                     static_cast<qreal>(otherY + TrackHeight));
        if (otherY < endY && blockedBottom > blockedTop) {
            blockedRanges.append({blockedTop, blockedBottom});
        }
    }

    std::sort(
        blockedRanges.begin(),
        blockedRanges.end(),
        [](const QPair<qreal, qreal> &left,
           const QPair<qreal, qreal> &right) {
            return left.first < right.first;
        });

    qreal segmentStart = startY;
    for (const QPair<qreal, qreal> &blocked : blockedRanges) {
        if (blocked.first > segmentStart) {
            segments.append(
                QLineF(x, segmentStart, x, blocked.first - 1.0));
        }
        segmentStart = std::max(segmentStart, blocked.second + 1.0);
        if (segmentStart >= endY) {
            break;
        }
    }
    if (segmentStart < endY) {
        segments.append(QLineF(x, segmentStart, x, endY));
    }
    return segments;
}

void GenomeAlignmentWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(viewport()->rect(), QColor(QStringLiteral("#171b1e")));

    painter.fillRect(QRect(0, 0, GutterWidth, viewport()->height()),
                     QColor(QStringLiteral("#202629")));
    painter.setPen(QPen(QColor(QStringLiteral("#384348")), 1));
    painter.drawLine(GutterWidth - 1, 0, GutterWidth - 1, viewport()->height());

    m_lastPaintedConnectionCount = 0;
    m_lastPaintedConnectionSegmentCount = 0;
    m_lastPaintedSelectedMismatchCount = 0;
    m_lastPaintedReadLabelCount = 0;
    m_readHitRegions.clear();

    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        painter.setPen(QColor(QStringLiteral("#8b989e")));
        painter.drawText(viewport()->rect(), Qt::AlignCenter,
                         QStringLiteral("加载或生成参考基因组后开始比对"));
        return;
    }

    const QString &reference = m_session->Reference();
    const int visibleStart = std::max(0, ViewportStart() - 1);
    const int visibleEnd =
        std::min(static_cast<int>(reference.size()), ViewportEnd() + 1);
    const bool drawCells = m_pixelsPerBase >= 3.0;
    const bool drawLetters = m_pixelsPerBase >= 10.0;
    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    baseFont.setPointSize(9);
    painter.setFont(baseFont);

    painter.setPen(QColor(QStringLiteral("#aeb9bd")));
    painter.drawText(QRect(12, ReferenceTop, GutterWidth - 22, TrackHeight),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Reference"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    const int tickStep =
        m_pixelsPerBase >= 12.0 ? 10
                               : m_pixelsPerBase >= 4.0 ? 25 : 100;
    const int firstTick = (visibleStart / tickStep) * tickStep;
    painter.save();
    painter.setClipRect(
        QRect(GutterWidth,
              0,
              viewport()->width() - GutterWidth,
              viewport()->height()));
    for (int position = firstTick; position <= visibleEnd; position += tickStep) {
        const qreal x = xForPosition(position);
        painter.setPen(QColor(QStringLiteral("#465258")));
        painter.drawLine(QPointF(x, 27.0),
                         QPointF(x, viewport()->height()));
        painter.setPen(QColor(QStringLiteral("#91a0a7")));
        painter.drawText(QRectF(x + 3.0, 8.0, 64.0, 17.0),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QString::number(position));
    }

    if (!drawCells) {
        const qreal startX = xForPosition(visibleStart);
        const qreal endX = xForPosition(visibleEnd);
        painter.fillRect(QRectF(startX,
                                ReferenceTop + 8,
                                endX - startX,
                                14),
                         QColor(QStringLiteral("#53656d")));
    } else {
        painter.setFont(baseFont);
        for (int position = visibleStart; position < visibleEnd; ++position) {
            const QRectF cell(xForPosition(position),
                              ReferenceTop,
                              m_pixelsPerBase,
                              TrackHeight);
            painter.fillRect(cell,
                             position % 2 == 0
                                 ? QColor(QStringLiteral("#263034"))
                                 : QColor(QStringLiteral("#222a2e")));
            painter.setPen(QPen(QColor(QStringLiteral("#3a464b")), 1));
            painter.drawRect(cell);
            if (drawLetters) {
                painter.setPen(QColor(QStringLiteral("#dbe3e6")));
                painter.drawText(cell, Qt::AlignCenter, reference.at(position));
            }
        }
    }
    painter.restore();

    const int selectedRead = m_session->SelectedReadIndex();
    const int selectedMatch = m_session->SelectedMatchIndex();
    const ReadLayoutItem *selectedLayout =
        layoutItemFor(selectedRead, selectedMatch);

    if (selectedLayout != nullptr && drawCells) {
        const ReadResultViewData *read =
            m_session->ReadAt(selectedLayout->readIndex);
        if (read != nullptr
            && selectedLayout->matchIndex >= 0
            && selectedLayout->matchIndex < read->matches.size()) {
            const MatchViewData &match =
                read->matches.at(selectedLayout->matchIndex);
            const int selectedY =
                ReadTop + selectedLayout->lane * ReadLaneHeight
                - verticalScrollBar()->value();
            if (match.End() > visibleStart
                && match.start < visibleEnd
                && selectedY > ReferenceTop + TrackHeight
                && selectedY < viewport()->height()) {
                painter.save();
                painter.setClipRect(
                    QRect(GutterWidth,
                          ReferenceTop + TrackHeight,
                          viewport()->width() - GutterWidth,
                          selectedY - ReferenceTop - TrackHeight));
                painter.setPen(
                    QPen(QColor(105, 204, 190, 62), 1, Qt::SolidLine));
                const int readStart =
                    std::max(0, visibleStart - match.start);
                const int readEnd =
                    std::min(match.length, visibleEnd - match.start);
                for (int offset = readStart; offset < readEnd; ++offset) {
                    if (hasMismatchAt(match, offset)) {
                        continue;
                    }
                    const int absolutePosition = match.start + offset;
                    const qreal x =
                        xForPosition(absolutePosition)
                        + m_pixelsPerBase / 2.0;
                    const QVector<QLineF> segments =
                        connectorSegments(
                            absolutePosition,
                            *selectedLayout,
                            x,
                            ReferenceTop + TrackHeight + 1,
                            selectedY - 1);
                    if (!segments.isEmpty()) {
                        painter.drawLines(segments);
                        ++m_lastPaintedConnectionCount;
                        m_lastPaintedConnectionSegmentCount +=
                            static_cast<int>(segments.size());
                    }
                }
                painter.restore();
            }
        }
    }

    int visibleReadCount = 0;
    for (const ReadLayoutItem &item : m_readLayout) {
        const ReadResultViewData *read = m_session->ReadAt(item.readIndex);
        if (read == nullptr
            || item.matchIndex < 0
            || item.matchIndex >= read->matches.size()) {
            continue;
        }
        const MatchViewData &match = read->matches.at(item.matchIndex);
        if (match.End() <= visibleStart || match.start >= visibleEnd) {
            continue;
        }

        const int y = ReadTop + item.lane * ReadLaneHeight
            - verticalScrollBar()->value();
        if (y + TrackHeight < ReadTop || y >= viewport()->height()) {
            continue;
        }
        ++visibleReadCount;
        const bool selected =
            item.readIndex == selectedRead
            && item.matchIndex == selectedMatch;
        if (selected) {
            m_lastPaintedSelectedMismatchCount =
                static_cast<int>(match.mismatches.size());
        }

        const QRectF entireRead(xForPosition(match.start),
                                y,
                                match.length * m_pixelsPerBase,
                                TrackHeight);
        painter.save();
        painter.setClipRect(
            QRect(GutterWidth,
                  ReadTop,
                  viewport()->width() - GutterWidth,
                  viewport()->height() - ReadTop));
        QColor background(QStringLiteral("#28765e"));
        background.setAlpha(selected ? 218 : 162);
        painter.fillRect(entireRead, background);
        painter.setPen(
            QPen(selected ? QColor(QStringLiteral("#69a9ff"))
                          : QColor(QStringLiteral("#4c9b82")),
                 selected ? 2.0 : 1.0));
        painter.drawRect(entireRead);

        const int readStart =
            std::max(0, visibleStart - match.start);
        const int readEnd =
            std::min(match.length, visibleEnd - match.start);
        if (drawCells) {
            painter.setFont(baseFont);
            for (int offset = readStart; offset < readEnd; ++offset) {
                const bool mismatch = hasMismatchAt(match, offset);
                const QRectF cell(xForPosition(match.start + offset),
                                  y,
                                  m_pixelsPerBase,
                                  TrackHeight);
                if (mismatch) {
                    painter.fillRect(
                        cell, QColor(QStringLiteral("#e2aa3d")));
                    painter.setPen(
                        QPen(QColor(QStringLiteral("#ff6e6e")), 2));
                    painter.drawRect(cell.adjusted(1, 1, -1, -1));
                    painter.setBrush(
                        QColor(QStringLiteral("#ff6e6e")));
                    const qreal center = cell.center().x();
                    const QPolygonF marker{
                        QPointF(center - 4, cell.bottom() + 1),
                        QPointF(center + 4, cell.bottom() + 1),
                        QPointF(center, cell.bottom() + 7)};
                    painter.drawPolygon(marker);
                    painter.setBrush(Qt::NoBrush);
                } else {
                    painter.setPen(
                        QPen(QColor(255, 255, 255, 32), 1));
                    painter.drawRect(cell);
                }
                if (drawLetters && offset < read->sequence.size()) {
                    painter.setPen(
                        mismatch
                            ? QColor(QStringLiteral("#35140d"))
                            : QColor(QStringLiteral("#f4f7f8")));
                    painter.drawText(cell,
                                     Qt::AlignCenter,
                                     read->sequence.at(offset));
                }
            }
        } else {
            for (const MismatchViewData &mismatch : match.mismatches) {
                if (mismatch.absolutePosition < visibleStart
                    || mismatch.absolutePosition >= visibleEnd) {
                    continue;
                }
                const QRectF marker(
                    xForPosition(mismatch.absolutePosition),
                    y,
                    std::max(2.0, m_pixelsPerBase),
                    TrackHeight);
                painter.fillRect(
                    marker, QColor(QStringLiteral("#e05e62")));
            }
        }
        painter.restore();

        if (selected) {
            painter.save();
            painter.setClipRect(
                QRect(0,
                      ReadTop,
                      GutterWidth,
                      viewport()->height() - ReadTop));
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
            painter.setPen(QColor(QStringLiteral("#7db2ff")));
            painter.drawText(
                QRect(12, y - 2, GutterWidth - 22, TrackHeight + 4),
                Qt::AlignLeft | Qt::AlignVCenter,
                QStringLiteral("Read #%1\n%2 · d=%3")
                    .arg(read->readId)
                    .arg(match.start)
                    .arg(match.hammingDistance));
            painter.restore();
            ++m_lastPaintedReadLabelCount;
        }

        const QRectF hitRect =
            entireRead.intersected(
                QRectF(GutterWidth,
                       ReadTop,
                       viewport()->width() - GutterWidth,
                       viewport()->height() - ReadTop));
        if (!hitRect.isEmpty()) {
            m_readHitRegions.append(
                {hitRect, item.readIndex, item.matchIndex});
        }
    }

    if (visibleReadCount == 0) {
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
        painter.setPen(QColor(QStringLiteral("#738188")));
        painter.drawText(QRect(GutterWidth,
                               ReadTop,
                               viewport()->width() - GutterWidth,
                               80),
                         Qt::AlignCenter,
                         QStringLiteral("当前窗口没有成功匹配的 Read"));
    }
}

void GenomeAlignmentWidget::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
    emitViewportChanged();
}

void GenomeAlignmentWidget::wheelEvent(QWheelEvent *event)
{
    if ((event->modifiers() & Qt::ControlModifier) != 0) {
        const double factor = event->angleDelta().y() > 0 ? 1.18 : 1.0 / 1.18;
        setPixelsPerBase(m_pixelsPerBase * factor, event->position().x());
        event->accept();
        return;
    }

    const int delta =
        event->angleDelta().x() != 0
            ? event->angleDelta().x()
            : event->angleDelta().y();
    horizontalScrollBar()->setValue(
        horizontalScrollBar()->value() - delta);
    event->accept();
}

void GenomeAlignmentWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragMoved = false;
        m_dragOrigin = event->position().toPoint();
        m_scrollOrigin = horizontalScrollBar()->value();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void GenomeAlignmentWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        const int distance = event->position().toPoint().x() - m_dragOrigin.x();
        if (std::abs(distance) > 3) {
            m_dragMoved = true;
        }
        horizontalScrollBar()->setValue(m_scrollOrigin - distance);
        event->accept();
        return;
    }
    showTooltipAt(event->position(), event->globalPosition().toPoint());
    QAbstractScrollArea::mouseMoveEvent(event);
}

void GenomeAlignmentWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        viewport()->setCursor(Qt::OpenHandCursor);
        if (!m_dragMoved) {
            bool activated = false;
            for (auto iterator = m_readHitRegions.crbegin();
                iterator != m_readHitRegions.crend();
                 ++iterator) {
                if (iterator->rect.contains(event->position())) {
                    if (m_matchActivatedCallback) {
                        m_matchActivatedCallback(iterator->readIndex,
                                                 iterator->matchIndex);
                    }
                    activated = true;
                    break;
                }
            }
            if (!activated && m_selectionClearedCallback) {
                m_selectionClearedCallback();
            }
        }
        event->accept();
        return;
    }
    QAbstractScrollArea::mouseReleaseEvent(event);
}

void GenomeAlignmentWidget::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    QAbstractScrollArea::leaveEvent(event);
}

void GenomeAlignmentWidget::showTooltipAt(const QPointF &point,
                                          const QPoint &globalPoint)
{
    if (m_session == nullptr) {
        return;
    }
    const int absolutePosition = genomePositionAt(point.x());
    if (absolutePosition < 0) {
        return;
    }

    for (auto iterator = m_readHitRegions.crbegin();
         iterator != m_readHitRegions.crend();
         ++iterator) {
        if (!iterator->rect.contains(point)) {
            continue;
        }
        const ReadResultViewData &read =
            m_session->Reads().at(iterator->readIndex);
        const MatchViewData &match =
            read.matches.at(iterator->matchIndex);
        const int offset = absolutePosition - match.start;
        if (offset < 0 || offset >= match.length
            || offset >= read.sequence.size()) {
            continue;
        }
        const QChar referenceBase =
            m_session->Reference().at(absolutePosition);
        const QChar readBase = read.sequence.at(offset);
        QToolTip::showText(
            globalPoint,
            QStringLiteral("Read #%1\n绝对位置：%2\nReference：%3\nRead：%4\n%5")
                .arg(read.readId)
                .arg(absolutePosition)
                .arg(referenceBase)
                .arg(readBase)
                .arg(referenceBase == readBase
                         ? QStringLiteral("碱基一致")
                         : QStringLiteral("容错错配")),
            viewport());
        return;
    }

    if (point.y() >= ReferenceTop
        && point.y() <= ReferenceTop + TrackHeight) {
        QToolTip::showText(
            globalPoint,
            QStringLiteral("参考基因组位置：%1\n碱基：%2")
                .arg(absolutePosition)
                .arg(m_session->Reference().at(absolutePosition)),
            viewport());
        return;
    }
    QToolTip::hideText();
}

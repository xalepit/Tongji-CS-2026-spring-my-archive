#include "GenomeOverviewWidget.h"

#include "SearchSessionModel.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

namespace {

QColor distanceColor(int distance)
{
    if (distance <= 0) {
        return QColor(QStringLiteral("#35b779"));
    }
    if (distance == 1) {
        return QColor(QStringLiteral("#e6a93d"));
    }
    return QColor(QStringLiteral("#e45b5b"));
}

} // namespace

GenomeOverviewWidget::GenomeOverviewWidget(QWidget *parent)
    : QWidget(parent),
      m_session(nullptr),
      m_viewStart(0),
      m_viewEnd(0)
{
    setMouseTracking(true);
    setMinimumHeight(100);
    setMaximumHeight(108);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void GenomeOverviewWidget::SetSession(const SearchSessionModel *session)
{
    m_session = session;
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        m_viewStart = 0;
        m_viewEnd = 0;
    } else if (m_viewEnd <= m_viewStart) {
        m_viewStart = 0;
        m_viewEnd =
            static_cast<int>(m_session->Reference().size());
    }
    update();
}

void GenomeOverviewWidget::SetViewportRange(int start, int end)
{
    if (m_session == nullptr) {
        return;
    }
    const int length =
        static_cast<int>(m_session->Reference().size());
    m_viewStart = std::clamp(start, 0, length);
    m_viewEnd = std::clamp(end, m_viewStart, length);
    update();
}

void GenomeOverviewWidget::Refresh()
{
    update();
}

void GenomeOverviewWidget::SetPositionActivatedCallback(
    const std::function<void(int)> &callback)
{
    m_positionActivatedCallback = callback;
}

void GenomeOverviewWidget::SetMatchActivatedCallback(
    const std::function<void(int, int)> &callback)
{
    m_matchActivatedCallback = callback;
}

QRectF GenomeOverviewWidget::contentRect() const
{
    return QRectF(24.0, 27.0, std::max(1, width() - 48), 16.0);
}

int GenomeOverviewWidget::positionAtX(qreal x) const
{
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        return 0;
    }
    const QRectF content = contentRect();
    const qreal ratio = std::clamp((x - content.left()) / content.width(), 0.0, 1.0);
    return static_cast<int>(
        ratio
        * static_cast<qreal>(m_session->Reference().size()));
}

void GenomeOverviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(QStringLiteral("#1b2023")));

    const QRectF content = contentRect();
    painter.setPen(QPen(QColor(QStringLiteral("#465258")), 1));
    painter.setBrush(QColor(QStringLiteral("#2b3439")));
    painter.drawRoundedRect(content, 3.0, 3.0);

    m_hitRegions.clear();
    if (m_session == nullptr || m_session->Reference().isEmpty()) {
        painter.setPen(QColor(QStringLiteral("#8d9aa0")));
        const QRectF messageRect(
            0.0, content.bottom() + 8.0, width(), 22.0);
        painter.drawText(messageRect, Qt::AlignCenter,
                         QStringLiteral("尚未加载参考基因组"));
        return;
    }

    const int genomeLength =
        static_cast<int>(m_session->Reference().size());
    const int selectedRead = m_session->SelectedReadIndex();
    const int selectedMatch = m_session->SelectedMatchIndex();
    constexpr int LaneCount = 5;
    QVector<qreal> laneEnds(LaneCount, content.left() - 4.0);

    for (int readIndex = 0; readIndex < m_session->Reads().size(); ++readIndex) {
        const ReadResultViewData &read = m_session->Reads().at(readIndex);
        for (int matchIndex = 0; matchIndex < read.matches.size(); ++matchIndex) {
            const MatchViewData &match = read.matches.at(matchIndex);
            const qreal startX =
                content.left() + (static_cast<qreal>(match.start) / genomeLength)
                    * content.width();
            const qreal proportionalWidth =
                (static_cast<qreal>(match.length) / genomeLength) * content.width();
            const qreal matchWidth = std::max(3.0, proportionalWidth);

            int lane = -1;
            for (int candidate = 0; candidate < LaneCount; ++candidate) {
                if (startX >= laneEnds.at(candidate) + 2.0) {
                    lane = candidate;
                    break;
                }
            }
            if (lane < 0) {
                lane = (readIndex + matchIndex) % LaneCount;
            }
            laneEnds[lane] = std::max(laneEnds.at(lane), startX + matchWidth);

            const QRectF matchRect(startX,
                                   50.0 + lane * 8.0,
                                   matchWidth,
                                   6.0);
            QColor color = distanceColor(match.hammingDistance);
            if (readIndex != selectedRead || matchIndex != selectedMatch) {
                color.setAlpha(205);
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(matchRect, 2.0, 2.0);

            if (readIndex == selectedRead && matchIndex == selectedMatch) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(QStringLiteral("#66a7ff")), 2));
                painter.drawRoundedRect(matchRect.adjusted(-2, -2, 2, 2), 3.0, 3.0);
            }
            m_hitRegions.append({matchRect.adjusted(-2, -2, 2, 2),
                                 readIndex,
                                 matchIndex});
        }
    }

    if (m_viewEnd > m_viewStart) {
        const qreal viewportX =
            content.left() + (static_cast<qreal>(m_viewStart) / genomeLength)
                * content.width();
        const qreal viewportWidth =
            std::max(3.0,
                     (static_cast<qreal>(m_viewEnd - m_viewStart) / genomeLength)
                         * content.width());
        const QRectF viewportRect(viewportX, 20.0, viewportWidth, 70.0);
        painter.setBrush(QColor(66, 139, 245, 38));
        painter.setPen(QPen(QColor(QStringLiteral("#5795ec")), 1.5));
        painter.drawRoundedRect(viewportRect, 3.0, 3.0);
    }

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    painter.setPen(QColor(QStringLiteral("#91a0a7")));
    constexpr int TickCount = 6;
    for (int tick = 0; tick <= TickCount; ++tick) {
        const qreal ratio = static_cast<qreal>(tick) / TickCount;
        const qreal x = content.left() + ratio * content.width();
        const int position = static_cast<int>(ratio * genomeLength);
        painter.drawLine(QPointF(x, 17.0), QPointF(x, 24.0));
        const QRectF labelRect =
            tick == 0
                ? QRectF(content.left(), 3.0, 60.0, 15.0)
                : tick == TickCount
                    ? QRectF(content.right() - 60.0, 3.0, 60.0, 15.0)
                    : QRectF(x - 30.0, 3.0, 60.0, 15.0);
        const Qt::Alignment alignment =
            tick == 0 ? Qt::AlignLeft
                      : tick == TickCount ? Qt::AlignRight : Qt::AlignCenter;
        painter.drawText(labelRect,
                         static_cast<int>(alignment),
                         QString::number(position));
    }

    painter.setPen(QColor(QStringLiteral("#748289")));
    painter.drawText(QRectF(content.left(), 86.0, content.width(), 14.0),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("%1 bp").arg(genomeLength));
}

void GenomeOverviewWidget::mouseMoveEvent(QMouseEvent *event)
{
    for (auto iterator = m_hitRegions.crbegin();
         iterator != m_hitRegions.crend();
         ++iterator) {
        if (!iterator->rect.contains(event->position())) {
            continue;
        }
        const ReadResultViewData *read = m_session->ReadAt(iterator->readIndex);
        if (read == nullptr || iterator->matchIndex >= read->matches.size()) {
            break;
        }
        const MatchViewData &match = read->matches.at(iterator->matchIndex);
        QToolTip::showText(
            event->globalPosition().toPoint(),
            QStringLiteral("Read #%1\n起点：%2\n终点：%3\n长度：%4 bp\n汉明距离：%5")
                .arg(read->readId)
                .arg(match.start)
                .arg(match.End() - 1)
                .arg(match.length)
                .arg(match.hammingDistance),
            this);
        return;
    }
    QToolTip::hideText();
}

void GenomeOverviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_session == nullptr) {
        QWidget::mousePressEvent(event);
        return;
    }

    for (auto iterator = m_hitRegions.crbegin();
        iterator != m_hitRegions.crend();
         ++iterator) {
        if (iterator->rect.contains(event->position())) {
            if (m_matchActivatedCallback) {
                m_matchActivatedCallback(iterator->readIndex,
                                         iterator->matchIndex);
            }
            return;
        }
    }
    if (m_positionActivatedCallback) {
        m_positionActivatedCallback(positionAtX(event->position().x()));
    }
}

void GenomeOverviewWidget::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    QWidget::leaveEvent(event);
}

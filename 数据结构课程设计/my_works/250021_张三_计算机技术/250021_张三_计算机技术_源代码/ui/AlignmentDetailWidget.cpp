#include "AlignmentDetailWidget.h"

#include "SearchSessionModel.h"

#include <QFontDatabase>
#include <QPainter>
#include <QScrollBar>

#include <algorithm>

namespace {

const MismatchViewData *mismatchAt(const MatchViewData &match, int offset)
{
    for (const MismatchViewData &mismatch : match.mismatches) {
        if (mismatch.offset == offset) {
            return &mismatch;
        }
    }
    return nullptr;
}

} // namespace

AlignmentDetailWidget::AlignmentDetailWidget(QWidget *parent)
    : QAbstractScrollArea(parent), m_session(nullptr)
{
    setFrameShape(QFrame::NoFrame);
    setAccessibleName(QStringLiteral("精确比对详情"));
    setProperty("detailRowCount", 3);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumHeight(130);
    setMaximumHeight(138);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            viewport(), qOverload<>(&QWidget::update));
}

void AlignmentDetailWidget::SetSession(const SearchSessionModel *session)
{
    m_session = session;
    updateAccessibleDescription();
    updateScrollRange();
    focusFirstMismatch();
    viewport()->update();
}

void AlignmentDetailWidget::Refresh()
{
    updateAccessibleDescription();
    updateScrollRange();
    focusFirstMismatch();
    viewport()->update();
}

void AlignmentDetailWidget::updateAccessibleDescription()
{
    const ReadResultViewData *read =
        m_session == nullptr ? nullptr : m_session->SelectedRead();
    const MatchViewData *match =
        m_session == nullptr ? nullptr : m_session->SelectedMatch();
    if (read == nullptr) {
        setAccessibleDescription(QStringLiteral("尚未选择 Read"));
    } else if (match == nullptr) {
        setAccessibleDescription(
            QStringLiteral("Read #%1 没有成功匹配").arg(read->readId));
    } else {
        setAccessibleDescription(
            QStringLiteral("Read #%1，匹配起点 %2，汉明距离 %3")
                .arg(read->readId)
                .arg(match->start)
                .arg(match->hammingDistance));
    }
}

void AlignmentDetailWidget::focusFirstMismatch()
{
    const MatchViewData *match =
        m_session == nullptr ? nullptr : m_session->SelectedMatch();
    if (match == nullptr || match->mismatches.isEmpty()) {
        horizontalScrollBar()->setValue(0);
        return;
    }
    const int available = std::max(1, viewport()->width() - GutterWidth);
    const int offset = match->mismatches.first().offset;
    horizontalScrollBar()->setValue(
        offset * CellWidth - available / 2 + CellWidth / 2);
}

void AlignmentDetailWidget::updateScrollRange()
{
    int sequenceLength = 0;
    if (m_session != nullptr && m_session->SelectedRead() != nullptr) {
        sequenceLength =
            static_cast<int>(
                m_session->SelectedRead()->sequence.size());
    }
    const int available = std::max(1, viewport()->width() - GutterWidth);
    horizontalScrollBar()->setRange(
        0, std::max(0, sequenceLength * CellWidth - available));
    horizontalScrollBar()->setPageStep(available);
    horizontalScrollBar()->setSingleStep(CellWidth * 3);
}

void AlignmentDetailWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(viewport()->rect(), QColor(QStringLiteral("#1b2023")));
    painter.fillRect(QRect(0, 0, GutterWidth, viewport()->height()),
                     QColor(QStringLiteral("#202629")));
    painter.setPen(QPen(QColor(QStringLiteral("#384348")), 1));
    painter.drawLine(GutterWidth - 1, 0, GutterWidth - 1,
                     viewport()->height());

    const ReadResultViewData *read =
        m_session == nullptr ? nullptr : m_session->SelectedRead();
    const MatchViewData *match =
        m_session == nullptr ? nullptr : m_session->SelectedMatch();
    if (read == nullptr || match == nullptr) {
        painter.setPen(QColor(QStringLiteral("#8c999f")));
        painter.drawText(viewport()->rect(),
                         Qt::AlignCenter,
                         read == nullptr
                             ? QStringLiteral("选择一个 Read 查看精确比对详情")
                             : QStringLiteral("当前 Read 没有成功匹配"));
        return;
    }

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
    painter.setPen(QColor(QStringLiteral("#c9d3d7")));
    painter.drawText(
        QRect(GutterWidth + 10, 3, viewport()->width() - GutterWidth - 20, 24),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("Read #%1  ·  最佳位置 %2  ·  长度 %3 bp  ·  "
                       "允许错配 %4  ·  实际汉明距离 %5  ·  匹配成功")
            .arg(read->readId)
            .arg(match->start)
            .arg(match->length)
            .arg(m_session->MaxMismatch())
            .arg(match->hammingDistance));

    const QString labels[] = {
        QStringLiteral("Reference"),
        QStringLiteral("Read"),
        QStringLiteral("比对关系")};
    const int rowTops[] = {29, 59, 89};
    for (int row = 0; row < 3; ++row) {
        painter.setPen(row == 2 ? QColor(QStringLiteral("#e6a93d"))
                                : QColor(QStringLiteral("#aeb9bd")));
        painter.drawText(QRect(12, rowTops[row], GutterWidth - 22, RowHeight),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         labels[row]);
    }

    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    baseFont.setPointSize(10);
    const int scroll = horizontalScrollBar()->value();
    const int firstOffset = std::max(0, scroll / CellWidth);
    const int lastOffset =
        std::min(match->length,
                 (scroll + viewport()->width() - GutterWidth) / CellWidth + 2);

    painter.save();
    painter.setClipRect(
        QRect(GutterWidth,
              0,
              viewport()->width() - GutterWidth,
              viewport()->height()));
    for (int offset = firstOffset; offset < lastOffset; ++offset) {
        const int x = GutterWidth + offset * CellWidth - scroll;
        const MismatchViewData *mismatch = mismatchAt(*match, offset);
        const bool isMismatch = mismatch != nullptr;

        for (int row = 0; row < 3; ++row) {
            const QRect cell(x, rowTops[row], CellWidth, RowHeight);
            QColor background =
                row % 2 == 0 ? QColor(QStringLiteral("#232b2f"))
                             : QColor(QStringLiteral("#20272a"));
            if (isMismatch && row <= 1) {
                background = QColor(QStringLiteral("#d49a37"));
            } else if (isMismatch && row == 2) {
                background = QColor(QStringLiteral("#3b2929"));
            }
            painter.fillRect(cell, background);
            painter.setPen(QPen(QColor(QStringLiteral("#3b474c")), 1));
            painter.drawRect(cell);
        }

        painter.setFont(baseFont);
        painter.setPen(isMismatch ? QColor(QStringLiteral("#28140c"))
                                  : QColor(QStringLiteral("#e4eaec")));
        painter.drawText(QRect(x, rowTops[0], CellWidth, RowHeight),
                         Qt::AlignCenter,
                         m_session->Reference().at(match->start + offset));
        painter.drawText(QRect(x, rowTops[1], CellWidth, RowHeight),
                         Qt::AlignCenter,
                         read->sequence.at(offset));

        if (isMismatch) {
            QFont relationFont(QStringLiteral("Consolas"), 6);
            relationFont.setBold(true);
            painter.setFont(relationFont);
            painter.setPen(QColor(QStringLiteral("#ffc36a")));
            painter.drawText(
                QRect(x, rowTops[2], CellWidth, RowHeight),
                Qt::AlignCenter,
                QStringLiteral("%1→%2\n@%3")
                    .arg(mismatch->referenceBase)
                    .arg(mismatch->readBase)
                    .arg(mismatch->absolutePosition));
        } else {
            QFont relationFont = baseFont;
            relationFont.setBold(true);
            painter.setFont(relationFont);
            painter.setPen(QColor(QStringLiteral("#43c596")));
            painter.drawText(
                QRect(x, rowTops[2], CellWidth, RowHeight),
                Qt::AlignCenter,
                QStringLiteral("│"));
        }
    }
    painter.restore();
}

void AlignmentDetailWidget::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

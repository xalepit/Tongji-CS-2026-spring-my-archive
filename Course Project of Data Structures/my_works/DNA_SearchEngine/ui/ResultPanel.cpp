#include "ResultPanel.h"

#include "SearchSessionModel.h"

#include <QAbstractItemView>
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QTableWidgetItem *centeredItem(const QString &text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

QString readIdText(int readId)
{
    return readId > 0 ? QStringLiteral("#%1").arg(readId)
                      : QStringLiteral("自定义");
}

} // namespace

ResultPanel::ResultPanel(QWidget *parent)
    : QWidget(parent),
      m_session(nullptr),
      m_currentAvailableReadIndex(-1),
      m_tabs(nullptr),
      m_readsSummaryLabel(nullptr),
      m_readSequenceLabel(nullptr),
      m_readMetadataLabel(nullptr),
      m_summaryLabel(nullptr),
      m_currentLabel(nullptr),
      m_availableReadTable(nullptr),
      m_resultTable(nullptr),
      m_candidateTable(nullptr)
{
    setObjectName(QStringLiteral("resultPanel"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(9);

    QLabel *panelTitle = new QLabel(QStringLiteral("Reads 与检索结果"));
    panelTitle->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(panelTitle);

    m_tabs = new QTabWidget;
    m_tabs->setObjectName(QStringLiteral("rightPanelTabs"));
    m_tabs->setDocumentMode(true);
    m_tabs->tabBar()->setExpanding(true);
    m_tabs->tabBar()->setUsesScrollButtons(false);

    QWidget *readsTab = new QWidget;
    QVBoxLayout *readsLayout = new QVBoxLayout(readsTab);
    readsLayout->setContentsMargins(0, 8, 0, 2);
    readsLayout->setSpacing(7);
    m_readsSummaryLabel = new QLabel(QStringLiteral("尚未生成或导入 Reads"));
    m_readsSummaryLabel->setObjectName(QStringLiteral("subtleText"));
    readsLayout->addWidget(m_readsSummaryLabel);

    m_availableReadTable = new QTableWidget(0, 4);
    m_availableReadTable->setObjectName(QStringLiteral("availableReadTable"));
    m_availableReadTable->setHorizontalHeaderLabels(
        {QStringLiteral("Read"),
         QStringLiteral("序列预览"),
         QStringLiteral("长度"),
         QStringLiteral("突变")});
    m_availableReadTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    m_availableReadTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    m_availableReadTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    m_availableReadTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    m_availableReadTable->verticalHeader()->setVisible(false);
    m_availableReadTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    m_availableReadTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    m_availableReadTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    m_availableReadTable->setAlternatingRowColors(true);
    readsLayout->addWidget(m_availableReadTable, 3);

    QFrame *readDetails = new QFrame;
    readDetails->setObjectName(QStringLiteral("readDetails"));
    QVBoxLayout *readDetailsLayout = new QVBoxLayout(readDetails);
    readDetailsLayout->setContentsMargins(9, 7, 9, 7);
    readDetailsLayout->setSpacing(4);
    m_readSequenceLabel =
        new QLabel(QStringLiteral("选择一条 Read 查看完整序列"));
    m_readSequenceLabel->setObjectName(QStringLiteral("readSequenceDetail"));
    m_readSequenceLabel->setWordWrap(true);
    m_readSequenceLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse);
    m_readMetadataLabel = new QLabel;
    m_readMetadataLabel->setObjectName(QStringLiteral("readMetadata"));
    m_readMetadataLabel->setWordWrap(true);
    readDetailsLayout->addWidget(m_readSequenceLabel);
    readDetailsLayout->addWidget(m_readMetadataLabel);
    readsLayout->addWidget(readDetails, 1);
    m_tabs->addTab(readsTab, QStringLiteral("Reads 列表"));

    QWidget *resultsTab = new QWidget;
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsTab);
    resultsLayout->setContentsMargins(0, 8, 0, 2);
    resultsLayout->setSpacing(7);
    m_summaryLabel = new QLabel(QStringLiteral("尚未运行检索"));
    m_summaryLabel->setObjectName(QStringLiteral("subtleText"));
    resultsLayout->addWidget(m_summaryLabel);

    m_resultTable = new QTableWidget(0, 4);
    m_resultTable->setObjectName(QStringLiteral("resultTable"));
    m_resultTable->setHorizontalHeaderLabels(
        {QStringLiteral("Read"),
         QStringLiteral("最佳位置"),
         QStringLiteral("距离"),
         QStringLiteral("状态")});
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    resultsLayout->addWidget(m_resultTable, 1);
    m_tabs->addTab(resultsTab, QStringLiteral("检索结果"));
    layout->addWidget(m_tabs, 3);

    QLabel *candidateTitle =
        new QLabel(QStringLiteral("当前 Read 的匹配位置"));
    candidateTitle->setObjectName(QStringLiteral("panelTitle"));
    m_currentLabel = new QLabel(QStringLiteral("选择一条 Read"));
    m_currentLabel->setObjectName(QStringLiteral("subtleText"));
    layout->addWidget(candidateTitle);
    layout->addWidget(m_currentLabel);

    m_candidateTable = new QTableWidget(0, 4);
    m_candidateTable->setObjectName(QStringLiteral("candidateTable"));
    m_candidateTable->setHorizontalHeaderLabels(
        {QStringLiteral("#"),
         QStringLiteral("起点"),
         QStringLiteral("距离"),
         QStringLiteral("局部片段")});
    m_candidateTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    m_candidateTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_candidateTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    m_candidateTable->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch);
    m_candidateTable->verticalHeader()->setVisible(false);
    m_candidateTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_candidateTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_candidateTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_candidateTable, 2);

    connect(m_availableReadTable, &QTableWidget::cellClicked,
            this, [this](int row, int) {
                m_currentAvailableReadIndex = row;
                populateReadDetails();
                if (m_availableReadActivatedCallback) {
                    m_availableReadActivatedCallback(row);
                }
            });
    connect(m_availableReadTable, &QTableWidget::cellDoubleClicked,
            this, [this](int row, int) {
                m_currentAvailableReadIndex = row;
                populateReadDetails();
                if (m_availableReadSearchRequestedCallback) {
                    m_availableReadSearchRequestedCallback(row);
                }
            });
    connect(m_resultTable, &QTableWidget::cellClicked,
            this, [this](int row, int) {
                if (m_session == nullptr) {
                    return;
                }
                const ReadResultViewData *read = m_session->ReadAt(row);
                if (m_readActivatedCallback) {
                    m_readActivatedCallback(
                        row,
                        read != nullptr && read->IsMatched() ? 0 : -1);
                }
            });
    connect(m_candidateTable, &QTableWidget::cellClicked,
            this, [this](int row, int) {
                if (m_session == nullptr
                    || m_session->SelectedReadIndex() < 0) {
                    return;
                }
                if (m_matchActivatedCallback) {
                    m_matchActivatedCallback(
                        m_session->SelectedReadIndex(), row);
                }
            });
}

void ResultPanel::SetSession(const SearchSessionModel *session)
{
    m_session = session;
    Refresh();
}

void ResultPanel::SetAvailableReads(
    const QVector<ReadInputViewData> &reads)
{
    m_availableReads = reads;
    if (m_currentAvailableReadIndex >= m_availableReads.size()) {
        m_currentAvailableReadIndex = -1;
    }
    populateAvailableReads();
    populateReadDetails();
}

void ResultPanel::SetCurrentAvailableReadIndex(int index)
{
    m_currentAvailableReadIndex =
        index >= 0 && index < m_availableReads.size() ? index : -1;
    populateAvailableReads();
    populateReadDetails();
    populateCandidates();
}

void ResultPanel::ShowReadsTab()
{
    m_tabs->setCurrentIndex(0);
}

void ResultPanel::ShowResultsTab()
{
    m_tabs->setCurrentIndex(1);
}

void ResultPanel::Refresh()
{
    populateAvailableReads();
    populateReadDetails();
    populateResults();
    populateCandidates();
}

void ResultPanel::SetAvailableReadActivatedCallback(
    const std::function<void(int)> &callback)
{
    m_availableReadActivatedCallback = callback;
}

void ResultPanel::SetAvailableReadSearchRequestedCallback(
    const std::function<void(int)> &callback)
{
    m_availableReadSearchRequestedCallback = callback;
}

void ResultPanel::SetReadActivatedCallback(
    const std::function<void(int, int)> &callback)
{
    m_readActivatedCallback = callback;
}

void ResultPanel::SetMatchActivatedCallback(
    const std::function<void(int, int)> &callback)
{
    m_matchActivatedCallback = callback;
}

void ResultPanel::populateAvailableReads()
{
    m_availableReadTable->blockSignals(true);
    m_availableReadTable->setRowCount(
        static_cast<int>(m_availableReads.size()));
    for (int row = 0; row < m_availableReads.size(); ++row) {
        const ReadInputViewData &read = m_availableReads.at(row);
        QString preview = read.sequence.left(15);
        if (read.sequence.size() > preview.size()) {
            preview.append(QStringLiteral("…"));
        }
        m_availableReadTable->setItem(
            row, 0, centeredItem(readIdText(read.readId)));
        m_availableReadTable->setItem(
            row, 1, new QTableWidgetItem(preview));
        m_availableReadTable->setItem(
            row, 2, centeredItem(QString::number(read.sequence.size())));
        m_availableReadTable->setItem(
            row, 3,
            centeredItem(read.sourcePosition >= 0
                             ? QString::number(read.mutations.size())
                             : QStringLiteral("—")));
        m_availableReadTable->setRowHeight(row, 29);
    }
    m_readsSummaryLabel->setText(
        m_availableReads.isEmpty()
            ? QStringLiteral("尚未生成或导入 Reads")
            : QStringLiteral("共 %1 条 · 单击选择，双击直接检索")
                  .arg(m_availableReads.size()));
    if (m_currentAvailableReadIndex >= 0) {
        m_availableReadTable->selectRow(m_currentAvailableReadIndex);
    } else {
        m_availableReadTable->clearSelection();
        m_availableReadTable->setCurrentCell(-1, -1);
    }
    m_availableReadTable->blockSignals(false);
}

void ResultPanel::populateReadDetails()
{
    if (m_currentAvailableReadIndex < 0
        || m_currentAvailableReadIndex >= m_availableReads.size()) {
        m_readSequenceLabel->setText(
            QStringLiteral("选择一条 Read 查看完整序列"));
        m_readMetadataLabel->clear();
        return;
    }

    const ReadInputViewData &read =
        m_availableReads.at(m_currentAvailableReadIndex);
    m_readSequenceLabel->setText(
        QStringLiteral("%1  %2")
            .arg(readIdText(read.readId))
            .arg(read.sequence));

    if (read.sourcePosition < 0) {
        m_readMetadataLabel->setText(
            QStringLiteral("来源位置：—\n突变信息：导入文件未提供"));
        return;
    }

    QString mutationText;
    for (const ReadMutationViewData &mutation : read.mutations) {
        if (!mutationText.isEmpty()) {
            mutationText.append(QStringLiteral("；"));
        }
        mutationText.append(
            QStringLiteral("偏移 %1 / 绝对 %2：%3→%4")
                .arg(mutation.offset)
                .arg(mutation.absolutePosition)
                .arg(mutation.referenceBase)
                .arg(mutation.readBase));
    }
    m_readMetadataLabel->setText(
        QStringLiteral("来源位置：%1\n突变：%2")
            .arg(read.sourcePosition)
            .arg(mutationText.isEmpty() ? QStringLiteral("无")
                                        : mutationText));
}

void ResultPanel::populateResults()
{
    m_resultTable->blockSignals(true);
    m_resultTable->setRowCount(0);
    if (m_session == nullptr) {
        m_summaryLabel->setText(QStringLiteral("尚未运行检索"));
        m_resultTable->blockSignals(false);
        return;
    }

    int matchedCount = 0;
    double totalMicroseconds = 0.0;
    const QVector<ReadResultViewData> &reads = m_session->Reads();
    m_resultTable->setRowCount(static_cast<int>(reads.size()));
    for (int row = 0; row < reads.size(); ++row) {
        const ReadResultViewData &read = reads.at(row);
        const MatchViewData *best =
            read.matches.isEmpty() ? nullptr : &read.matches.first();
        if (best != nullptr) {
            ++matchedCount;
        }
        totalMicroseconds += read.elapsedMicroseconds;

        m_resultTable->setItem(
            row, 0, centeredItem(readIdText(read.readId)));
        m_resultTable->setItem(
            row, 1,
            centeredItem(best == nullptr ? QStringLiteral("—")
                                         : QString::number(best->start)));
        m_resultTable->setItem(
            row, 2,
            centeredItem(best == nullptr
                             ? QStringLiteral("—")
                             : QString::number(best->hammingDistance)));
        QTableWidgetItem *status =
            centeredItem(best == nullptr ? QStringLiteral("未命中")
                                         : QStringLiteral("匹配"));
        status->setForeground(best == nullptr
                                  ? QColor(QStringLiteral("#929da2"))
                                  : QColor(QStringLiteral("#46c892")));
        m_resultTable->setItem(row, 3, status);
        m_resultTable->setRowHeight(row, 29);
    }

    m_summaryLabel->setText(
        reads.isEmpty()
            ? QStringLiteral("尚未运行检索")
            : QStringLiteral("%1 / %2 条成功 · 总检索耗时 %3 μs")
                  .arg(matchedCount)
                  .arg(reads.size())
                  .arg(totalMicroseconds, 0, 'f', 2));
    if (m_session->SelectedReadIndex() >= 0) {
        m_resultTable->selectRow(m_session->SelectedReadIndex());
    } else {
        m_resultTable->clearSelection();
        m_resultTable->setCurrentCell(-1, -1);
    }
    m_resultTable->blockSignals(false);
}

void ResultPanel::populateCandidates()
{
    m_candidateTable->blockSignals(true);
    m_candidateTable->setRowCount(0);
    const ReadResultViewData *read =
        m_session == nullptr ? nullptr : m_session->SelectedRead();
    if (read == nullptr) {
        if (m_currentAvailableReadIndex >= 0
            && m_currentAvailableReadIndex < m_availableReads.size()) {
            const ReadInputViewData &available =
                m_availableReads.at(m_currentAvailableReadIndex);
            const int resultIndex =
                m_session == nullptr
                    ? -1
                    : m_session->FindReadIndexById(available.readId);
            read = m_session == nullptr
                ? nullptr
                : m_session->ReadAt(resultIndex);
            if (read == nullptr) {
                m_currentLabel->setText(
                    QStringLiteral("%1 · 尚未检索")
                        .arg(readIdText(available.readId)));
            }
        } else {
            m_currentLabel->setText(QStringLiteral("选择一条 Read"));
        }
        if (read == nullptr) {
            m_candidateTable->blockSignals(false);
            return;
        }
    }

    m_currentLabel->setText(
        QStringLiteral("%1 · 种子候选 %2 · 成功位置 %3")
            .arg(readIdText(read->readId))
            .arg(read->candidateCount)
            .arg(read->matches.size()));
    m_candidateTable->setRowCount(
        static_cast<int>(read->matches.size()));
    for (int row = 0; row < read->matches.size(); ++row) {
        const MatchViewData &match = read->matches.at(row);
        const QString preview =
            m_session->Reference().mid(
                match.start, std::min(match.length, 18));
        m_candidateTable->setItem(
            row, 0, centeredItem(QString::number(row + 1)));
        m_candidateTable->setItem(
            row, 1, centeredItem(QString::number(match.start)));
        m_candidateTable->setItem(
            row, 2, centeredItem(QString::number(match.hammingDistance)));
        m_candidateTable->setItem(
            row, 3, new QTableWidgetItem(preview));
        m_candidateTable->setRowHeight(row, 29);
    }
    if (m_session->SelectedMatchIndex() >= 0) {
        m_candidateTable->selectRow(m_session->SelectedMatchIndex());
    } else {
        m_candidateTable->clearSelection();
        m_candidateTable->setCurrentCell(-1, -1);
    }
    m_candidateTable->blockSignals(false);
}

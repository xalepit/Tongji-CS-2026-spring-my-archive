#ifndef RESULTPANEL_H
#define RESULTPANEL_H

#include "AlignmentViewData.h"

#include <QVector>
#include <QWidget>

#include <functional>

class QLabel;
class QTabWidget;
class QTableWidget;
class SearchSessionModel;

class ResultPanel : public QWidget
{
public:
    explicit ResultPanel(QWidget *parent = nullptr);

    void SetSession(const SearchSessionModel *session);
    void SetAvailableReads(const QVector<ReadInputViewData> &reads);
    void SetCurrentAvailableReadIndex(int index);
    void ShowReadsTab();
    void ShowResultsTab();
    void Refresh();

    void SetAvailableReadActivatedCallback(
        const std::function<void(int)> &callback);
    void SetAvailableReadSearchRequestedCallback(
        const std::function<void(int)> &callback);
    void SetReadActivatedCallback(
        const std::function<void(int, int)> &callback);
    void SetMatchActivatedCallback(
        const std::function<void(int, int)> &callback);

private:
    const SearchSessionModel *m_session;
    QVector<ReadInputViewData> m_availableReads;
    int m_currentAvailableReadIndex;
    QTabWidget *m_tabs;
    QLabel *m_readsSummaryLabel;
    QLabel *m_readSequenceLabel;
    QLabel *m_readMetadataLabel;
    QLabel *m_summaryLabel;
    QLabel *m_currentLabel;
    QTableWidget *m_availableReadTable;
    QTableWidget *m_resultTable;
    QTableWidget *m_candidateTable;
    std::function<void(int)> m_availableReadActivatedCallback;
    std::function<void(int)> m_availableReadSearchRequestedCallback;
    std::function<void(int, int)> m_readActivatedCallback;
    std::function<void(int, int)> m_matchActivatedCallback;

    void populateAvailableReads();
    void populateReadDetails();
    void populateResults();
    void populateCandidates();
};

#endif // RESULTPANEL_H

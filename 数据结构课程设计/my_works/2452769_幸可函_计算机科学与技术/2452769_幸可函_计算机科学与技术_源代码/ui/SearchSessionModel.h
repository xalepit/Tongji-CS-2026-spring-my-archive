#ifndef SEARCHSESSIONMODEL_H
#define SEARCHSESSIONMODEL_H

#include "AlignmentViewData.h"

#include <QString>
#include <QVector>

class SearchResult;

class SearchSessionModel
{
public:
    SearchSessionModel();

    void SetReference(const QString &reference);
    void SetMaxMismatch(int maxMismatch);
    void ClearResults();
    void AddResult(int readId, int sourcePosition, const SearchResult &result);

    const QString &Reference() const;
    int MaxMismatch() const;
    const QVector<ReadResultViewData> &Reads() const;
    const ReadResultViewData *ReadAt(int index) const;
    const MatchViewData *SelectedMatch() const;
    const ReadResultViewData *SelectedRead() const;

    int SelectedReadIndex() const;
    int SelectedMatchIndex() const;
    int FindReadIndexById(int readId) const;
    bool Select(int readIndex, int matchIndex);
    bool SelectFirstAvailable();
    void ClearSelection();

private:
    QString m_reference;
    QVector<ReadResultViewData> m_reads;
    int m_maxMismatch;
    int m_selectedReadIndex;
    int m_selectedMatchIndex;
};

#endif // SEARCHSESSIONMODEL_H

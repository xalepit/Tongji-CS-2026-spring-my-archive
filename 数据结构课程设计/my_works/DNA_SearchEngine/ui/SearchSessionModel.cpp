#include "SearchSessionModel.h"

#include "core/DnaLimits.h"
#include "core/SearchResult.h"

#include <algorithm>
#include <stdexcept>

SearchSessionModel::SearchSessionModel()
    : m_maxMismatch(0),
      m_selectedReadIndex(-1),
      m_selectedMatchIndex(-1)
{
}

void SearchSessionModel::SetReference(const QString &reference)
{
    m_reference = reference;
    ClearResults();
}

void SearchSessionModel::SetMaxMismatch(int maxMismatch)
{
    if (maxMismatch < DnaLimits::MinMismatchCount
        || maxMismatch > DnaLimits::MaxMismatchCount) {
        throw std::invalid_argument(DnaLimits::MismatchCountError);
    }
    m_maxMismatch = maxMismatch;
}

void SearchSessionModel::ClearResults()
{
    m_reads.clear();
    m_selectedReadIndex = -1;
    m_selectedMatchIndex = -1;
}

void SearchSessionModel::AddResult(int readId,
                                   int sourcePosition,
                                   const SearchResult &result)
{
    ReadResultViewData readView;
    readView.readId = readId;
    readView.sequence =
        QString::fromLatin1(result.Read().Bases(), result.Read().Length());
    readView.sourcePosition = sourcePosition;
    readView.candidateCount = result.CandidateCount();
    readView.elapsedMicroseconds = result.ElapsedMicroseconds();
    readView.matches.reserve(result.MatchCount());

    for (int matchIndex = 0; matchIndex < result.MatchCount(); ++matchIndex) {
        const SearchMatch &match = result.MatchAt(matchIndex);
        MatchViewData matchView;
        matchView.start = match.Position();
        matchView.length = result.Read().Length();
        matchView.hammingDistance = match.MismatchCount();
        matchView.mismatches.reserve(match.MismatchCount());

        for (int mismatchIndex = 0;
             mismatchIndex < match.MismatchCount();
             ++mismatchIndex) {
            const int offset = match.MismatchOffset(mismatchIndex);
            const int absolutePosition = match.Position() + offset;
            MismatchViewData mismatch;
            mismatch.offset = offset;
            mismatch.absolutePosition = absolutePosition;
            if (absolutePosition >= 0 && absolutePosition < m_reference.size()) {
                mismatch.referenceBase = m_reference.at(absolutePosition);
            }
            if (offset >= 0 && offset < readView.sequence.size()) {
                mismatch.readBase = readView.sequence.at(offset);
            }
            matchView.mismatches.append(mismatch);
        }
        readView.matches.append(matchView);
    }

    std::sort(readView.matches.begin(),
              readView.matches.end(),
              [](const MatchViewData &left, const MatchViewData &right) {
                  if (left.hammingDistance != right.hammingDistance) {
                      return left.hammingDistance < right.hammingDistance;
                  }
                  return left.start < right.start;
              });
    for (int index = 0; index < m_reads.size(); ++index) {
        if (m_reads.at(index).readId != readId) {
            continue;
        }
        m_reads[index] = readView;
        if (m_selectedReadIndex == index) {
            m_selectedMatchIndex =
                readView.matches.isEmpty() ? -1 : 0;
        }
        return;
    }
    m_reads.append(readView);
}

const QString &SearchSessionModel::Reference() const
{
    return m_reference;
}

int SearchSessionModel::MaxMismatch() const
{
    return m_maxMismatch;
}

const QVector<ReadResultViewData> &SearchSessionModel::Reads() const
{
    return m_reads;
}

const ReadResultViewData *SearchSessionModel::ReadAt(int index) const
{
    if (index < 0 || index >= m_reads.size()) {
        return nullptr;
    }
    return &m_reads.at(index);
}

const MatchViewData *SearchSessionModel::SelectedMatch() const
{
    const ReadResultViewData *read = SelectedRead();
    if (read == nullptr
        || m_selectedMatchIndex < 0
        || m_selectedMatchIndex >= read->matches.size()) {
        return nullptr;
    }
    return &read->matches.at(m_selectedMatchIndex);
}

const ReadResultViewData *SearchSessionModel::SelectedRead() const
{
    return ReadAt(m_selectedReadIndex);
}

int SearchSessionModel::SelectedReadIndex() const
{
    return m_selectedReadIndex;
}

int SearchSessionModel::SelectedMatchIndex() const
{
    return m_selectedMatchIndex;
}

int SearchSessionModel::FindReadIndexById(int readId) const
{
    for (int index = 0; index < m_reads.size(); ++index) {
        if (m_reads.at(index).readId == readId) {
            return index;
        }
    }
    return -1;
}

bool SearchSessionModel::Select(int readIndex, int matchIndex)
{
    const ReadResultViewData *read = ReadAt(readIndex);
    if (read == nullptr) {
        return false;
    }
    if (matchIndex < -1 || matchIndex >= read->matches.size()) {
        return false;
    }
    m_selectedReadIndex = readIndex;
    m_selectedMatchIndex = matchIndex;
    return true;
}

bool SearchSessionModel::SelectFirstAvailable()
{
    if (m_reads.isEmpty()) {
        m_selectedReadIndex = -1;
        m_selectedMatchIndex = -1;
        return false;
    }

    for (int index = 0; index < m_reads.size(); ++index) {
        if (!m_reads.at(index).matches.isEmpty()) {
            m_selectedReadIndex = index;
            m_selectedMatchIndex = 0;
            return true;
        }
    }

    m_selectedReadIndex = 0;
    m_selectedMatchIndex = -1;
    return true;
}

void SearchSessionModel::ClearSelection()
{
    m_selectedReadIndex = -1;
    m_selectedMatchIndex = -1;
}

#ifndef ALIGNMENTVIEWDATA_H
#define ALIGNMENTVIEWDATA_H

#include <QChar>
#include <QString>
#include <QVector>

struct MismatchViewData
{
    int offset = -1;
    int absolutePosition = -1;
    QChar referenceBase;
    QChar readBase;
};

struct ReadMutationViewData
{
    int offset = -1;
    int absolutePosition = -1;
    QChar referenceBase;
    QChar readBase;
};

struct ReadInputViewData
{
    int readId = -1;
    QString sequence;
    int sourcePosition = -1;
    QVector<ReadMutationViewData> mutations;
};

struct MatchViewData
{
    int start = -1;
    int length = 0;
    int hammingDistance = 0;
    QVector<MismatchViewData> mismatches;

    int End() const
    {
        return start + length;
    }
};

struct ReadResultViewData
{
    int readId = -1;
    QString sequence;
    int sourcePosition = -1;
    int candidateCount = 0;
    double elapsedMicroseconds = 0.0;
    QVector<MatchViewData> matches;

    bool IsMatched() const
    {
        return !matches.isEmpty();
    }
};

#endif // ALIGNMENTVIEWDATA_H

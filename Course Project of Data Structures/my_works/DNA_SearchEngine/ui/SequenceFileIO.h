#ifndef SEQUENCEFILEIO_H
#define SEQUENCEFILEIO_H

#include <QString>
#include <QVector>

class SearchSessionModel;

class SequenceFileIO
{
public:
    static bool LoadReference(const QString &path,
                              QString *reference,
                              QString *errorMessage);
    static bool LoadReads(const QString &path,
                          QVector<QString> *reads,
                          QString *errorMessage);
    static bool ExportResults(const QString &path,
                              const SearchSessionModel &session,
                              QString *errorMessage);
    static QString EscapeCsvField(const QString &text);

private:
    static bool IsDna(const QString &sequence);
};

#endif // SEQUENCEFILEIO_H

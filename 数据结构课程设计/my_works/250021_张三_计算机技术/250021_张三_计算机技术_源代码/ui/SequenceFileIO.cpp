#include "SequenceFileIO.h"

#include "SearchSessionModel.h"

#include <QFile>
#include <QTextStream>

namespace {

QString normalizedSequence(QString value)
{
    value.remove(QChar(0xFEFF));
    value.remove(' ');
    value.remove('\t');
    value.remove('\r');
    return value.trimmed().toUpper();
}

QString mismatchText(const MatchViewData &match)
{
    QString result;
    for (const MismatchViewData &mismatch : match.mismatches) {
        if (!result.isEmpty()) {
            result.append(';');
        }
        result.append(QStringLiteral("%1:%2>%3")
                          .arg(mismatch.absolutePosition)
                          .arg(mismatch.referenceBase)
                          .arg(mismatch.readBase));
    }
    return result;
}

} // namespace

bool SequenceFileIO::LoadReference(const QString &path,
                                   QString *reference,
                                   QString *errorMessage)
{
    errorMessage->clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = QStringLiteral("无法打开参考基因组文件");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString sequence;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('>')) {
            continue;
        }
        sequence.append(normalizedSequence(line));
    }

    if (sequence.isEmpty()) {
        *errorMessage = QStringLiteral("文件中没有可用的 DNA 序列");
        return false;
    }
    if (!IsDna(sequence)) {
        *errorMessage = QStringLiteral("参考基因组只能包含 A、T、C、G");
        return false;
    }
    *reference = sequence;
    return true;
}

bool SequenceFileIO::LoadReads(const QString &path,
                               QVector<QString> *reads,
                               QString *errorMessage)
{
    errorMessage->clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = QStringLiteral("无法打开 Reads 文件");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QVector<QString> parsed;
    QString current;
    bool formatKnown = false;
    bool fasta = false;
    bool sawHeader = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        line.remove(QChar(0xFEFF));
        line = line.trimmed();
        if (line.isEmpty()) {
            if (formatKnown && !fasta) {
                *errorMessage =
                    QStringLiteral("纯文本 Reads 文件中存在空 Read");
                return false;
            }
            continue;
        }
        if (line.startsWith('>')) {
            if (!formatKnown) {
                formatKnown = true;
                fasta = true;
            } else if (!fasta) {
                *errorMessage =
                    QStringLiteral("Reads 文件不能混合纯文本和 FASTA 格式");
                return false;
            }
            if (sawHeader && current.isEmpty()) {
                *errorMessage =
                    QStringLiteral("FASTA 标题后缺少 Read 序列");
                return false;
            }
            if (sawHeader) {
                parsed.append(normalizedSequence(current));
                current.clear();
            }
            sawHeader = true;
            continue;
        }
        if (!formatKnown) {
            formatKnown = true;
            fasta = false;
        }
        if (fasta) {
            current.append(line);
        } else {
            parsed.append(normalizedSequence(line));
        }
    }
    if (fasta) {
        if (!sawHeader || current.isEmpty()) {
            *errorMessage =
                QStringLiteral("FASTA 标题后缺少 Read 序列");
            return false;
        }
        parsed.append(normalizedSequence(current));
    }

    if (parsed.isEmpty()) {
        *errorMessage = QStringLiteral("文件中没有可用的 Read");
        return false;
    }
    for (int index = 0; index < parsed.size(); ++index) {
        if (!IsDna(parsed.at(index))) {
            *errorMessage =
                QStringLiteral("第 %1 条 Read 包含 A、T、C、G 以外的字符")
                    .arg(index + 1);
            return false;
        }
    }
    *reads = parsed;
    return true;
}

bool SequenceFileIO::ExportResults(const QString &path,
                                   const SearchSessionModel &session,
                                   QString *errorMessage)
{
    errorMessage->clear();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        *errorMessage = QStringLiteral("无法创建结果文件");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream.setGenerateByteOrderMark(true);
    stream << "Read编号,序列,匹配排名,匹配位置,汉明距离,"
              "错配详情,状态,候选位置数,耗时（微秒）\n";

    for (const ReadResultViewData &read : session.Reads()) {
        if (read.matches.isEmpty()) {
            stream << read.readId << ','
                   << EscapeCsvField(read.sequence)
                   << ",—,—,—,\"\",未命中,"
                   << read.candidateCount << ','
                   << QString::number(
                          read.elapsedMicroseconds, 'f', 2) << '\n';
            continue;
        }

        for (int index = 0; index < read.matches.size(); ++index) {
            const MatchViewData &match = read.matches.at(index);
            stream << read.readId << ','
                   << EscapeCsvField(read.sequence) << ','
                   << index + 1 << ','
                   << match.start << ','
                   << match.hammingDistance << ','
                   << EscapeCsvField(mismatchText(match)) << ",匹配,"
                   << read.candidateCount << ','
                   << QString::number(
                          read.elapsedMicroseconds, 'f', 2) << '\n';
        }
    }
    stream.flush();
    if (stream.status() != QTextStream::Ok || !file.flush()) {
        *errorMessage = QStringLiteral("写入结果文件失败");
        return false;
    }
    return true;
}

bool SequenceFileIO::IsDna(const QString &sequence)
{
    if (sequence.isEmpty()) {
        return false;
    }
    for (const QChar base : sequence) {
        if (base != 'A' && base != 'T' && base != 'C' && base != 'G') {
            return false;
        }
    }
    return true;
}

QString SequenceFileIO::EscapeCsvField(const QString &text)
{
    QString escaped = text;
    escaped.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

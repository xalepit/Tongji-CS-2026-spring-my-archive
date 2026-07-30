#include "SearchEngine.h"

#include <chrono>
#include <cstddef>
#include <random>
#include <stdexcept>

SearchEngine::SearchEngine()
    : _referenceGenome(),
      _hashTable(DEFAULT_HASH_CAPACITY),
      _k(DnaLimits::DefaultKmerLength),
      _maxMismatch(DnaLimits::DefaultMismatchCount),
      _lastResult(),
      _indexBuildMicroseconds(0)
{
    std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<int> lengthDistribution(
        DnaLimits::MinReferenceLength,
        DnaLimits::MaxReferenceLength);
    std::uniform_int_distribution<int> baseDistribution(0, 3);
    const char bases[4] = {'A', 'C', 'G', 'T'};
    const int length = lengthDistribution(randomEngine);

    char *generated =
        new char[static_cast<std::size_t>(length) + 1U];
    for (int i = 0; i < length; ++i) {
        generated[i] = bases[baseDistribution(randomEngine)];
    }
    generated[length] = '\0';
    _referenceGenome = Genome(generated, length);
    delete[] generated;

    IndexConstruct();
}

SearchEngine::SearchEngine(const Genome &referenceGenome)
    : SearchEngine(referenceGenome,
                   DnaLimits::DefaultKmerLength,
                   DnaLimits::DefaultMismatchCount,
                   DEFAULT_HASH_CAPACITY)
{
}

SearchEngine::SearchEngine(const Genome &referenceGenome,
                           int k,
                           int maxMismatch,
                           int capacity)
    : _referenceGenome(referenceGenome),
      _hashTable(capacity),
      _k(k),
      _maxMismatch(maxMismatch),
      _lastResult(),
      _indexBuildMicroseconds(0)
{
    if (_referenceGenome.Length() < DnaLimits::MinReferenceLength
        || _referenceGenome.Length() > DnaLimits::MaxReferenceLength) {
        throw std::invalid_argument(DnaLimits::ReferenceLengthError);
    }
    if (!IsDnaSequence(_referenceGenome)) {
        throw std::invalid_argument("Reference genome may contain only A, T, C and G");
    }
    if (_k < DnaLimits::MinKmerLength
        || _k > DnaLimits::MaxKmerLength) {
        throw std::invalid_argument(DnaLimits::KmerLengthError);
    }
    if (_maxMismatch < DnaLimits::MinMismatchCount
        || _maxMismatch > DnaLimits::MaxMismatchCount) {
        throw std::invalid_argument(DnaLimits::MismatchCountError);
    }

    IndexConstruct();
}

bool SearchEngine::IsDnaSequence(const Genome &sequence)
{
    for (int i = 0; i < sequence.Length(); ++i) {
        const char base = sequence.Bases()[i];
        if (base != 'A' && base != 'T' && base != 'C' && base != 'G') {
            return false;
        }
    }
    return true;
}

void SearchEngine::IndexConstruct()
{
    const auto started = std::chrono::steady_clock::now();
    const int lastStart = _referenceGenome.Length() - _k;
    // 逐位滑动窗口，索引位置总数严格为 N-K+1。
    for (int position = 0; position <= lastStart; ++position) {
        Genome key(_referenceGenome.Bases() + position, _k);
        _hashTable.Insert(key, position);
    }
    const auto finished = std::chrono::steady_clock::now();
    _indexBuildMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(finished - started).count();
}

void SearchEngine::Search(const Genome &read)
{
    if (read.Length() < DnaLimits::MinReadLength
        || read.Length() > DnaLimits::MaxReadLength) {
        throw std::invalid_argument(DnaLimits::ReadLengthError);
    }
    if (read.Length() < _k) {
        throw std::invalid_argument("Read length must be at least K");
    }
    if (read.Length() > _referenceGenome.Length()) {
        throw std::invalid_argument("Read cannot be longer than the reference genome");
    }
    if (!IsDnaSequence(read)) {
        throw std::invalid_argument("Read may contain only A, T, C and G");
    }

    _lastResult.Reset(read);
    const auto started = std::chrono::steady_clock::now();

    Genome seed(read.Bases(), _k);
    PositionList candidates = _hashTable.Find(seed);
    for (ListNode<int> *node = candidates; node != nullptr; node = node->_next) {
        _lastResult.AddCandidate();
        const int position = node->_data;
        if (position < 0 || position + read.Length() > _referenceGenome.Length()) {
            continue;
        }

        int *mismatchOffsets = _maxMismatch > 0 ? new int[_maxMismatch] : nullptr;
        int mismatchCount = 0;
        // 汉明距离只比较等长序列的同一偏移；超阈值立即剪枝。
        for (int offset = 0; offset < read.Length(); ++offset) {
            if (read.Bases()[offset] != _referenceGenome.Bases()[position + offset]) {
                if (mismatchCount < _maxMismatch) {
                    mismatchOffsets[mismatchCount] = offset;
                }
                ++mismatchCount;
                if (mismatchCount > _maxMismatch) {
                    break;
                }
            }
        }

        if (mismatchCount <= _maxMismatch) {
            _lastResult.AppendMatch(position, mismatchCount, mismatchOffsets);
        }
        delete[] mismatchOffsets;
    }

    const auto finished = std::chrono::steady_clock::now();
    _lastResult.SetElapsedMicroseconds(
        std::chrono::duration<double, std::micro>(finished - started).count());
}

void SearchEngine::SetMaxMismatch(int maxMismatch)
{
    if (maxMismatch < DnaLimits::MinMismatchCount
        || maxMismatch > DnaLimits::MaxMismatchCount) {
        throw std::invalid_argument(DnaLimits::MismatchCountError);
    }
    _maxMismatch = maxMismatch;
    _lastResult = SearchResult();
}

const Genome &SearchEngine::GetReferenceGenome() const
{
    return _referenceGenome;
}

const SearchResult &SearchEngine::GetLastResult() const
{
    return _lastResult;
}

int SearchEngine::GetK() const
{
    return _k;
}

int SearchEngine::GetMaxMismatch() const
{
    return _maxMismatch;
}

int SearchEngine::GetIndexedKmerCount() const
{
    return _hashTable.GetKeyCount();
}

int SearchEngine::GetIndexedPositionCount() const
{
    return _hashTable.GetPositionCount();
}

int SearchEngine::GetHashCapacity() const
{
    return _hashTable.GetCapacity();
}

int SearchEngine::GetCollisionCount() const
{
    return _hashTable.GetCollisionCount();
}

double SearchEngine::GetLoadFactor() const
{
    return _hashTable.GetLoadFactor();
}

long long SearchEngine::GetIndexBuildMicroseconds() const
{
    return _indexBuildMicroseconds;
}

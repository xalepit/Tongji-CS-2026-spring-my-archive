#include "ZipperHashTable.h"

#include <stdexcept>

HashEntry::HashEntry(const Genome &key)
    : _key(key), _positions(nullptr), _next(nullptr)
{
}

HashEntry::~HashEntry()
{
    while (_positions != nullptr) {
        ListNode<int> *node = _positions;
        _positions = _positions->_next;
        delete node;
    }
    _positions = nullptr;
}

ZipperHashTable::ZipperHashTable(int capacity)
    : _buckets(nullptr),
      _capacity(capacity),
      _keyCount(0),
      _positionCount(0),
      _collisionCount(0)
{
    if (_capacity <= 0) {
        throw std::invalid_argument("Hash capacity must be positive");
    }

    _buckets = new HashEntry *[_capacity];
    for (int i = 0; i < _capacity; ++i) {
        _buckets[i] = nullptr;
    }
}

ZipperHashTable::~ZipperHashTable()
{
    Clear();
    delete[] _buckets;
    _buckets = nullptr;
    _capacity = 0;
}

void ZipperHashTable::Clear()
{
    for (int i = 0; i < _capacity; ++i) {
        HashEntry *entry = _buckets[i];
        while (entry != nullptr) {
            HashEntry *next = entry->_next;
            delete entry;
            entry = next;
        }
        _buckets[i] = nullptr;
    }

    _keyCount = 0;
    _positionCount = 0;
    _collisionCount = 0;
}

int ZipperHashTable::Hash(const Genome &key) const
{
    unsigned long long value = 0;
    for (int i = 0; i < key.Length(); ++i) {
        unsigned int code = 0;
        switch (key.Bases()[i]) {
        case 'A': code = 1; break;
        case 'C': code = 2; break;
        case 'G': code = 3; break;
        case 'T': code = 4; break;
        default: code = static_cast<unsigned char>(key.Bases()[i]) + 5U; break;
        }
        value = (value * 131ULL + code) % static_cast<unsigned long long>(_capacity);
    }
    return static_cast<int>(value);
}

HashEntry *ZipperHashTable::FindEntry(const Genome &key)
{
    HashEntry *entry = _buckets[Hash(key)];
    while (entry != nullptr) {
        if (entry->_key == key) {
            return entry;
        }
        entry = entry->_next;
    }
    return nullptr;
}

void ZipperHashTable::InsertPosition(PositionList &list, int position)
{
    // 题目要求位置链表采用头插法。
    ListNode<int> *node = new ListNode<int>;
    node->_data = position;
    node->_next = list;
    list = node;
}

void ZipperHashTable::Insert(const Genome &key, int position)
{
    HashEntry *entry = FindEntry(key);
    if (entry == nullptr) {
        const int bucket = Hash(key);
        if (_buckets[bucket] != nullptr) {
            ++_collisionCount;
        }
        entry = new HashEntry(key);
        entry->_next = _buckets[bucket];
        _buckets[bucket] = entry;
        ++_keyCount;
    }

    InsertPosition(entry->_positions, position);
    ++_positionCount;
}

PositionList ZipperHashTable::Find(const Genome &key)
{
    HashEntry *entry = FindEntry(key);
    return entry == nullptr ? nullptr : entry->_positions;
}

int ZipperHashTable::GetCapacity() const
{
    return _capacity;
}

int ZipperHashTable::GetKeyCount() const
{
    return _keyCount;
}

int ZipperHashTable::GetPositionCount() const
{
    return _positionCount;
}

int ZipperHashTable::GetCollisionCount() const
{
    return _collisionCount;
}

double ZipperHashTable::GetLoadFactor() const
{
    return static_cast<double>(_keyCount) / static_cast<double>(_capacity);
}

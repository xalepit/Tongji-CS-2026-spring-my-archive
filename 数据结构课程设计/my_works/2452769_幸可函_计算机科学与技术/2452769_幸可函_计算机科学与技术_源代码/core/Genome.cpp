#include "Genome.h"

#include <cstddef>
#include <stdexcept>

Genome::Genome()
    : _length(0), _bases(new char[1])
{
    _bases[0] = '\0';
}

Genome::Genome(const char *bases, int length)
    : _length(length), _bases(nullptr)
{
    if (length < 0 || (length > 0 && bases == nullptr)) {
        throw std::invalid_argument("Invalid genome buffer or length");
    }

    _bases = new char[_length + 1];
    for (int i = 0; i < _length; ++i) {
        _bases[i] = bases[i];
    }
    _bases[_length] = '\0';
}

Genome::~Genome()
{
    delete[] _bases;
    _bases = nullptr;
    _length = 0;
}

Genome::Genome(const Genome &other)
    : _length(other._length),
      _bases(new char[static_cast<std::size_t>(other._length) + 1U])
{
    for (int i = 0; i < _length; ++i) {
        _bases[i] = other._bases[i];
    }
    _bases[_length] = '\0';
}

Genome &Genome::operator=(const Genome &other)
{
    if (this == &other) {
        return *this;
    }

    char *newBases =
        new char[static_cast<std::size_t>(other._length) + 1U];
    for (int i = 0; i < other._length; ++i) {
        newBases[i] = other._bases[i];
    }
    newBases[other._length] = '\0';

    delete[] _bases;
    _bases = newBases;
    _length = other._length;
    return *this;
}

bool Genome::operator==(const Genome &other) const
{
    if (_length != other._length) {
        return false;
    }
    for (int i = 0; i < _length; ++i) {
        if (_bases[i] != other._bases[i]) {
            return false;
        }
    }
    return true;
}

int Genome::Length() const
{
    return _length;
}

const char *Genome::Bases() const
{
    return _bases;
}

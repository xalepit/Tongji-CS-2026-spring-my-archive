#ifndef APPLICATIONLIMITS_H
#define APPLICATIONLIMITS_H

#include "core/DnaLimits.h"

namespace ApplicationLimits {

inline constexpr int MinReferenceLength = DnaLimits::MinReferenceLength;
inline constexpr int MaxReferenceLength = DnaLimits::MaxReferenceLength;
inline constexpr int MinReadLength = DnaLimits::MinReadLength;
inline constexpr int MaxReadLength = DnaLimits::MaxReadLength;
inline constexpr int MinKmerLength = DnaLimits::MinKmerLength;
inline constexpr int MaxKmerLength = DnaLimits::MaxKmerLength;
inline constexpr int MinMismatchCount = DnaLimits::MinMismatchCount;
inline constexpr int MaxMismatchCount = DnaLimits::MaxMismatchCount;
inline constexpr int MinReadCount = DnaLimits::MinReadCount;
inline constexpr int MaxReadCount = DnaLimits::MaxReadCount;

inline constexpr int DefaultReferenceLength = DnaLimits::DefaultReferenceLength;
inline constexpr int DefaultReadLength = DnaLimits::DefaultReadLength;
inline constexpr int DefaultKmerLength = DnaLimits::DefaultKmerLength;
inline constexpr int DefaultMismatchCount = DnaLimits::DefaultMismatchCount;
inline constexpr int DefaultReadCount = DnaLimits::DefaultReadCount;

inline constexpr int MinLogPanelHeight = 72;
inline constexpr int DefaultLogPanelHeight = 90;
inline constexpr int MaxLogPanelHeight = 140;

} // namespace ApplicationLimits

#endif // APPLICATIONLIMITS_H

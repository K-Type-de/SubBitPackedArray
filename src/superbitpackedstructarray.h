#ifndef _KT_SUPERBITPACKEDSTRUCTARRAY_H_
#define _KT_SUPERBITPACKEDSTRUCTARRAY_H_

#include <cstdint>

#include "subbitpackedstruct.h"
#include "subbitpackedstructarray.h"
#include "utils/superbitpackedarrayentrymetadata.h"

namespace kt
{
template <typename StructEntry, std::size_t num_entries>
class SuperBitPackedStructArray
{
  static constexpr uint8_t kBitsPerEntry = StructEntry::GetBitsUsed();
  static constexpr uint8_t kExtraBitsPerWord = 32 - kBitsPerEntry;

  static_assert(kExtraBitsPerWord > 0,
                "[SuperBitPackedStructArray] Cannot SuperBitPack structs with no extra bits. Use "
                "\"SubBitPackedStructArray\" instead");

  static constexpr std::size_t kEntrySize =
      (num_entries * kBitsPerEntry) / 32 +
      ((num_entries * kBitsPerEntry) % 32 !=
       0);  // Add +1 array entry if it does not already fit perfectly

  uint32_t storage_[kEntrySize];

  inline Internal::SuperBitPackedArrayEntryMetadata getEntryMetadata(std::size_t entry_index) const
  {
    const std::size_t bit_address = (entry_index * 32) - (entry_index * kExtraBitsPerWord);
    const std::size_t start_index = bit_address / 32;
    const std::size_t bit_shift = bit_address % 32;

    return {start_index, bit_shift};
  }

  inline StructEntry _getEntry(std::size_t start_index, std::size_t bit_shift) const
  {
    if (!bit_shift)
    {
      uint32_t entry = this->storage_[start_index];
      // Shift out upper bits
      entry <<= kExtraBitsPerWord;
      entry >>= kExtraBitsPerWord;
      return static_cast<StructEntry>(entry);
    }

    const uint32_t &entry_1 = this->storage_[start_index];
    const uint32_t &entry_2 = this->storage_[start_index + 1];

    uint32_t combined_entry = (entry_1 >> bit_shift) | (entry_2 << (32 - bit_shift));
    // Shift out upper bits
    combined_entry <<= kExtraBitsPerWord;
    combined_entry >>= kExtraBitsPerWord;
    return static_cast<StructEntry>(combined_entry);
  }

  inline void _setEntry(std::size_t start_index, std::size_t bit_shift, StructEntry entry)
  {
    if (!bit_shift)
    {
      // Clear old bits
      this->storage_[start_index] >>= kBitsPerEntry;
      this->storage_[start_index] <<= kBitsPerEntry;
      // Store new value
      this->storage_[start_index] |= entry.getData();
      return;
    }

    // Clear old bits
    this->storage_[start_index] <<= 32 - bit_shift;
    this->storage_[start_index] >>= 32 - bit_shift;
    this->storage_[start_index + 1] >>= bit_shift - 1;
    this->storage_[start_index + 1] <<= bit_shift - 1;

    // Store new value
    this->storage_[start_index] |= entry.getData() << bit_shift;
    this->storage_[start_index + 1] |= entry.getData() >> (32 - bit_shift);
  }

public:
  inline uint16_t getState(std::size_t entry_index, std::size_t state_index) const
  {
    auto metadata = this->getEntryMetadata(entry_index);
    const StructEntry entry = this->_getEntry(metadata.start_index, metadata.bit_shift);

    return entry.get(state_index);
  }

  inline void setState(std::size_t entry_index, std::size_t state_index, uint16_t state)
  {
    auto metadata = this->getEntryMetadata(entry_index);
    StructEntry entry = this->_getEntry(metadata.start_index, metadata.bit_shift);

    entry.set(state_index, state);

    this->_setEntry(metadata.start_index, metadata.bit_shift, entry);
  }
};

}  // namespace kt

#endif  // _KT_SUPERBITPACKEDSTRUCTARRAY_H_

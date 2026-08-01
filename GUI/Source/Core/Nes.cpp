#include "Core/Nes.hpp"

#include <array>
#include <fstream>
#include <limits>

namespace
{
  constexpr std::size_t HEADER_SIZE = 16;
  constexpr std::size_t TRAINER_SIZE = 512;
  constexpr std::size_t PRG_BANK_SIZE = 16 * 1024;
  constexpr std::size_t CHR_BANK_SIZE = 8 * 1024;

  bool checked_add(const std::size_t left, const std::size_t right, std::size_t &result)
  {
    if (left > std::numeric_limits<std::size_t>::max() - right)
      return false;
    result = left + right;
    return true;
  }

  bool checked_size(const std::uint64_t units, const std::size_t unitSize, std::uint64_t &result)
  {
    if (units > std::numeric_limits<std::uint64_t>::max() / unitSize)
      return false;
    result = units * unitSize;
    return true;
  }

  bool parse_header(const std::array<std::uint8_t, HEADER_SIZE> &bytes, Nes::Header &header, std::string &error)
  {
    constexpr std::array<std::uint8_t, 4> SIGNATURE = {'N', 'E', 'S', 0x1A};
    for (std::size_t i = 0; i < SIGNATURE.size(); ++i)
    {
      if (bytes[i] != SIGNATURE[i])
      {
        error = "Invalid iNES signature.";
        return false;
      }
    }

    header.version = (bytes[7] & 0x0C) == 0x08 ? Nes::HeaderVersion::Nes20 : Nes::HeaderVersion::INes;
    header.mapperId = static_cast<std::uint16_t>((bytes[6] >> 4) | (bytes[7] & 0xF0));
    header.hasBattery = (bytes[6] & 0x02) != 0;
    header.hasTrainer = (bytes[6] & 0x04) != 0;
    header.mirroring = (bytes[6] & 0x08) != 0
                           ? Nes::MirroringType::FourScreen
                           : ((bytes[6] & 0x01) != 0 ? Nes::MirroringType::Vertical : Nes::MirroringType::Horizontal);

    if (header.version == Nes::HeaderVersion::Nes20)
    {
      header.mapperId |= static_cast<std::uint16_t>(bytes[8] & 0x0F) << 8;
      header.submapperId = bytes[8] >> 4;

      const std::uint8_t prgMsb = bytes[9] & 0x0F;
      const std::uint8_t chrMsb = bytes[9] >> 4;
      if (prgMsb == 0x0F || chrMsb == 0x0F)
      {
        error = "NES 2.0 exponent/multiplier ROM sizes are not supported yet.";
        return false;
      }

      if (!checked_size((static_cast<std::uint64_t>(prgMsb) << 8) | bytes[4], PRG_BANK_SIZE, header.prgRomSize) ||
          !checked_size((static_cast<std::uint64_t>(chrMsb) << 8) | bytes[5], CHR_BANK_SIZE, header.chrRomSize))
      {
        error = "ROM size declared by the NES 2.0 header is too large.";
        return false;
      }
    }
    else
    {
      header.prgRomSize = static_cast<std::uint64_t>(bytes[4]) * PRG_BANK_SIZE;
      header.chrRomSize = static_cast<std::uint64_t>(bytes[5]) * CHR_BANK_SIZE;
    }

    return true;
  }

  void build_banks(std::vector<Nes::Bank> &banks, const Nes::RegionType type, const std::size_t offset,
                   const std::size_t totalSize, const std::size_t bankSize)
  {
    const std::size_t bankCount = totalSize / bankSize;
    banks.reserve(bankCount);
    for (std::size_t index = 0; index < bankCount; ++index)
      banks.push_back({type, index, offset + index * bankSize, bankSize, 0});
  }
}

namespace Nes
{
  void Document::analyze(const std::uint8_t value)
  {
    emptyValue = value;
    const auto analyzeBanks = [this](std::vector<Bank> &banks)
    {
      for (Bank &bank : banks)
      {
        bank.occupiedBytes = 0;
        for (std::size_t index = 0; index < bank.size; ++index)
          bank.occupiedBytes += bytes[bank.fileOffset + index] != emptyValue ? 1u : 0u;
      }
    };
    analyzeBanks(prgBanks);
    analyzeBanks(chrBanks);
  }

  ParseResult load_document(const std::filesystem::path &path)
  {
    ParseResult result;
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
      result.error = "Unable to open the file.";
      return result;
    }

    const std::streampos endPosition = stream.tellg();
    if (endPosition < static_cast<std::streampos>(HEADER_SIZE))
    {
      result.error = "The file is too short to contain an iNES header.";
      return result;
    }

    const auto fileSize = static_cast<std::uint64_t>(endPosition);
    if (fileSize > std::numeric_limits<std::size_t>::max())
    {
      result.error = "The file is too large for this build.";
      return result;
    }

    result.document.bytes.resize(static_cast<std::size_t>(fileSize));
    stream.seekg(0, std::ios::beg);
    if (!stream.read(reinterpret_cast<char *>(result.document.bytes.data()), static_cast<std::streamsize>(fileSize)))
    {
      result.error = "Unable to read the complete file.";
      result.document.bytes.clear();
      return result;
    }

    std::array<std::uint8_t, HEADER_SIZE> headerBytes{};
    for (std::size_t index = 0; index < HEADER_SIZE; ++index)
      headerBytes[index] = result.document.bytes[index];
    if (!parse_header(headerBytes, result.document.header, result.error))
      return result;

    if (result.document.header.prgRomSize > std::numeric_limits<std::size_t>::max() ||
        result.document.header.chrRomSize > std::numeric_limits<std::size_t>::max())
    {
      result.error = "The declared ROM data is too large for this build.";
      return result;
    }

    std::size_t prgOffset = HEADER_SIZE;
    if (result.document.header.hasTrainer && !checked_add(prgOffset, TRAINER_SIZE, prgOffset))
    {
      result.error = "Invalid trainer offset.";
      return result;
    }

    std::size_t chrOffset = 0;
    std::size_t expectedSize = 0;
    if (!checked_add(prgOffset, static_cast<std::size_t>(result.document.header.prgRomSize), chrOffset) ||
        !checked_add(chrOffset, static_cast<std::size_t>(result.document.header.chrRomSize), expectedSize) ||
        expectedSize > result.document.bytes.size())
    {
      result.error = "The file is truncated: PRG/CHR data is smaller than declared by its header.";
      return result;
    }

    if (expectedSize < result.document.bytes.size())
      result.warning = "The file contains trailing data that is not displayed.";

    result.document.path = path;
    build_banks(result.document.prgBanks, RegionType::Prg, prgOffset,
                static_cast<std::size_t>(result.document.header.prgRomSize), PRG_BANK_SIZE);
    build_banks(result.document.chrBanks, RegionType::Chr, chrOffset,
                static_cast<std::size_t>(result.document.header.chrRomSize), CHR_BANK_SIZE);
    result.document.analyze(0x00);
    result.success = true;
    return result;
  }

  const char *header_version_name(const HeaderVersion version)
  {
    return version == HeaderVersion::Nes20 ? "NES 2.0" : "iNES";
  }

  const char *mirroring_name(const MirroringType mirroring)
  {
    switch (mirroring)
    {
    case MirroringType::Vertical:
      return "Vertical";
    case MirroringType::FourScreen:
      return "Four-screen";
    default:
      return "Horizontal";
    }
  }
}

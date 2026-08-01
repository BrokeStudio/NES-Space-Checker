#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Nes
{
  enum class HeaderVersion
  {
    INes,
    Nes20
  };

  enum class MirroringType
  {
    Horizontal,
    Vertical,
    FourScreen
  };

  enum class RegionType
  {
    Prg,
    Chr
  };

  struct Header
  {
    HeaderVersion version = HeaderVersion::INes;
    MirroringType mirroring = MirroringType::Horizontal;
    std::uint16_t mapperId = 0;
    std::uint8_t submapperId = 0;
    std::uint64_t prgRomSize = 0;
    std::uint64_t chrRomSize = 0;
    bool hasBattery = false;
    bool hasTrainer = false;
  };

  struct Bank
  {
    RegionType type = RegionType::Prg;
    std::size_t index = 0;
    std::size_t fileOffset = 0;
    std::size_t size = 0;
    std::size_t occupiedBytes = 0;
  };

  struct Document
  {
    std::filesystem::path path;
    std::vector<std::uint8_t> bytes;
    Header header;
    std::vector<Bank> prgBanks;
    std::vector<Bank> chrBanks;
    std::uint8_t emptyValue = 0x00;

    void analyze(std::uint8_t value);
  };

  struct ParseResult
  {
    bool success = false;
    Document document;
    std::string error;
    std::string warning;
  };

  ParseResult load_document(const std::filesystem::path &path);
  const char *header_version_name(HeaderVersion version);
  const char *mirroring_name(MirroringType mirroring);
}

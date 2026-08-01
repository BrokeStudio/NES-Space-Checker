#pragma once
#include <cstdint>
#include <string>

namespace App
{
  struct Events
  {
    bool exit = false;
    bool showDemo = false;
    std::string setWindowTitle = "NES Space Checker";
  };

  Events render();
  void on_file_drop(const std::string &selectedPath);
}

// fake_engine_partial_failure.cpp - writes a corrupt partial then fails.
//
// Used by the offscreen GUI U-59 regression: it proves that a non-zero engine
// exit after touching the isolated write path cannot damage a pre-existing
// destination, and that the .gs-partial is removed.
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string output;
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == "-o") {
      output = argv[i + 1];
      break;
    }
  }
  if (output.empty()) return 64;
  std::ofstream file(output, std::ios::binary | std::ios::trunc);
  if (!file) return 65;
  file << "corrupt partial from failing engine";
  file.close();
  std::cerr << "corrupt partial fixture failure\n";
  return 7;
}

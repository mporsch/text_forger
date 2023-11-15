#include <clocale>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include "builder.h"
#include "literal.h"

int main(int argc, char** argv) {
  // set global locale
  std::setlocale(LC_ALL, "de_DE.UTF-8");

  if(argc < 2) {
    return EXIT_FAILURE;
  } else {
    // train word map from user input
    Builder builder;
    for(auto arg = argv + 1; arg < argv + argc; ++arg) {
      IFSTREAM ifs(*arg);
      if(!ifs.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(*arg));
      }
      ifs >> std::noskipws;
      builder.train(ifs);
    }
    auto forger = builder.get();

    // forge iterations based on user input
    for(;;) {
      int wordCount;
      std::cout << "How many words to generate? (empty for exit) - ";

      std::string line;
      std::getline(std::cin, line);
      if(line.empty()) {
        break;
      } else {
        std::istringstream iss(line);
        if (iss >> wordCount) {
          COUT << '\n' << forger(static_cast<size_t>(wordCount)) << LITERAL("\n\n");
        }
      }
    }

    return EXIT_SUCCESS;
  }
}

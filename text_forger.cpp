#include <csignal>
#include <iostream>
#include <vector>
#include <map>

struct WordReference {
  std::string word;
  unsigned count;
};
using WordReferenceList = std::vector<WordReference>;

struct WordMapEntry {
  WordReferenceList refs;
  unsigned refSum;
};
using WordMap = std::map<std::string, WordMapEntry>;

void TrainFrom(WordMap &wordMap, char const *arg) {

}

unsigned TrainFinalize(WordMap &wordMap) {

}

std::string Forge(unsigned wordCount) {

}

void Exit(int) {
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  // set up Ctrl-C handler
  if(std::signal(SIGINT, Exit)) {
    std::cerr << "Failed to register signal handler\n";
    return EXIT_FAILURE;
  }

  if(argc < 2) {
    return EXIT_FAILURE;
  } else {
    // train word map from user input
    WordMap wordMap;
    for(auto arg = argv[1]; arg < argv[argc]; ++arg) {
      TrainFrom(wordMap, arg);
    }
    auto const refCountSum = TrainFinalize(wordMap);
    std::cout << "Trained word map with "
      << wordMap.size() << " entries with "
      << refCountSum << " references\n\n";

    // forge iterations based on user input
    for(;;) {
      int wordCount;
      std::cout << "How many words to generate? - ";
      if(!(std::cin >> wordCount) ||
         (wordCount < 0)) {
        std::cout << "Invalid input\n\n";
      } else if(wordCount == 0) {
        break;
      } else {
        std::cout << '\n' << Forge(static_cast<unsigned>(wordCount)) << "\n\n";
      }
    }

    return EXIT_SUCCESS;
  }
}

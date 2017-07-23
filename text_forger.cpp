#include <csignal>
#include <iostream>
#include <map>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <numeric>

#define DEBUG_TRAINING

struct WordReference {
  unsigned count;
};
using WordReferenceMap = std::map<std::wstring, WordReference>;

struct WordMapEntry {
  WordReferenceMap refs;
  unsigned refSum;
};
using WordMap = std::map<std::wstring, WordMapEntry>;

enum CharType {
  Space
, Letter
, Number
, Special
};

CharType GetCharType(wchar_t c) {
  if(std::isspace(c)) {
    return Space;
  } else if(std::isalpha(c, std::locale())) {
    return Letter;
  } else if(std::isalnum(c)) {
    return Number;
  } else {
    return Special;
  }
}

void TrainFromLine(WordMap &wordMap, std::wstring const &line) {
  if(line.empty()) {
    return;
  }

  wordMap[L"\n"];
  std::wstring lastToken = L"\n";
  size_t tokenStart = 0;
  auto lastCharType = GetCharType(line.at(0));
  for(size_t pos = 1; pos < line.length(); ++pos) {
    auto const charType = GetCharType(line.at(pos));
    if(charType != lastCharType) {
      auto const token = line.substr(tokenStart, pos - tokenStart);

      if(!token.empty()) {
        wordMap[token];

        ++wordMap.at(lastToken).refs[token].count;

        lastToken = token;
      }

      tokenStart = pos + (charType == Space ? 1 : 0);
    }
    lastCharType = charType;
  }

  auto const token = line.substr(tokenStart);
  if(!token.empty()) {
    wordMap[token];
    ++wordMap.at(lastToken).refs[token].count;
  }
}

void TrainFrom(WordMap &wordMap, char const *arg) {
  std::wifstream ifs(arg);
  if(!ifs.is_open()) {
    throw std::runtime_error("Failed to open " + std::string(arg));
  }

  std::wstring line;
  while(std::getline(ifs, line)) {
    TrainFromLine(wordMap, line);
  }
}

unsigned TrainFinalize(WordMap &wordMap) {
  unsigned sumsum = 0;
  for(auto &&p : wordMap) {
    auto const sum = std::accumulate(
      std::begin(p.second.refs), std::end(p.second.refs), 0,
      [](unsigned sum, WordReferenceMap::value_type const &p) -> unsigned {
        return sum + p.second.count;
      });
      p.second.refSum = sum;
      sumsum += sum;
  }
  return sumsum;
}

void DumpWordMap(WordMap const &wordMap) {
  for(auto &&wordMapPair : wordMap) {
    std::wcout << wordMapPair.first << ": ";
    for(auto &&wordReferenceMapPair : wordMapPair.second.refs) {
      std::wcout << wordReferenceMapPair.first
        << " (" << wordReferenceMapPair.second.count << ") ";
    }
    std::wcout << '\n';
  }
}

std::wstring Forge(unsigned wordCount, WordMap const &wordMap) {
  return L"";
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

  // set global locale
  std::locale::global(std::locale("de_DE.UTF-8"));

  if(argc < 2) {
    return EXIT_FAILURE;
  } else {
    // train word map from user input
    WordMap wordMap;
    for(auto arg = argv + 1; arg < argv + argc; ++arg) {
      TrainFrom(wordMap, *arg);
    }
    auto const refCountSum = TrainFinalize(wordMap);
#ifdef DEBUG_TRAINING
    DumpWordMap(wordMap);
#endif
    std::wcout << "Trained word map with "
      << wordMap.size() << " entries with "
      << refCountSum << " references\n\n";

    // forge iterations based on user input
    for(;;) {
      int wordCount;
      std::wcout << "How many words to generate? - ";
      if(!(std::cin >> wordCount) ||
         (wordCount < 0)) {
        std::wcout << "Invalid input\n\n";
      } else if(wordCount == 0) {
        break;
      } else {
        std::wcout << '\n' << Forge(static_cast<unsigned>(wordCount), wordMap) << "\n\n";
      }
    }

    return EXIT_SUCCESS;
  }
}

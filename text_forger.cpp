#include <csignal>
#include <iostream>
#include <iterator>
#include <map>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <numeric>
#include <algorithm>
#include <random>

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

// globally used random generator
std::default_random_engine generator;

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

void TrainFromFile(WordMap &wordMap, std::wistream &is) {
  std::vector<std::wstring> tokens(4, L"\n");
  auto insertAndReference = [&](std::wstring const &token) {
    if(!token.empty()) {
      // insert into word map
      wordMap[token];

      // rotate and insert into ring of previous words
      std::rotate(std::begin(tokens), std::next(std::begin(tokens)), std::end(tokens));
      tokens[tokens.size() - 1] = token;

      // increase reference count in previous words
      auto prev = std::begin(tokens);
      for(auto curr = std::next(std::begin(tokens)); curr != std::end(tokens); ++curr) {
        ++wordMap.at(*prev).refs[*curr].count;
        prev = curr;
      }
    }
  };

  std::wstring token = L"\n";
  auto lastCharType = Special;
  using iterator = std::istream_iterator<wchar_t, wchar_t>;
  for(iterator it(is); it != iterator(); ++it) {

    auto const charType = GetCharType(*it);
    if(charType != lastCharType) {
      insertAndReference(token);
      token.clear();

      lastCharType = charType;
    }

    if(charType != Space) {
      token += *it;
    }
  }
  insertAndReference(token);
}

void TrainFrom(WordMap &wordMap, char const *arg) {
  std::wifstream ifs(arg);
  if(!ifs.is_open()) {
    throw std::runtime_error("Failed to open " + std::string(arg));
  }
  ifs >> std::noskipws;
  TrainFromFile(wordMap, ifs);
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

std::wstring GetRandomReference(WordMapEntry const &wordMapEntry) {
  if(wordMapEntry.refSum == 0) {
    return L"\n";
  } else {
    std::uniform_int_distribution<unsigned> distribution(0, wordMapEntry.refSum - 1);
    auto const random = distribution(generator);

    unsigned cdf = 0;
    for(auto &&ref : wordMapEntry.refs) {
      cdf += ref.second.count;
      if(random < cdf) {
        return ref.first;
      }
    }
    throw std::runtime_error("Invalid random " + std::to_string(random) +
      " of " + std::to_string(wordMapEntry.refSum));
  }
}

std::wstring GetInitialToken(WordMap const &wordMap) {
  if(wordMap.count(L"\n")) {
    return GetRandomReference(wordMap.at(L"\n"));
  } else {
    std::uniform_int_distribution<size_t> distribution(0, wordMap.size());
    return (std::next(std::begin(wordMap), distribution(generator)))->first;
  }
}

std::wstring Forge(unsigned wordCount, WordMap const &wordMap) {
  std::wstring ret;
  ret.reserve(wordCount * 10);

  auto token = GetInitialToken(wordMap);
  while(wordCount > 0)
  {
    token = GetRandomReference(wordMap.at(token));
    ret += L" " + token;
    --wordCount;
  }

  return ret;
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
      if(!(std::cin >> wordCount)
      || (wordCount < 0)) {
        std::wcout << "Invalid input\n\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      } else if(wordCount == 0) {
        break;
      } else {
        std::wcout << '\n' << Forge(static_cast<unsigned>(wordCount), wordMap) << "\n\n";
      }
    }

    return EXIT_SUCCESS;
  }
}

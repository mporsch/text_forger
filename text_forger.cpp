#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

//#define DEBUG_TOKENIZE
//#define DEBUG_TRAINING

static const unsigned markovChainOrder = 2;

struct Forger {
  friend struct Builder;

  std::wstring generate(size_t count) const {
    Tokens tokens;
    tokens.reserve(count);
    {
      // initially add order+1 tokens
      auto ts = first();
      tokens.insert(end(tokens), begin(ts), end(ts));

      // subsequently add only the follow-up state token
      while(tokens.size() < count) {
        ts = next(ts);
        tokens.push_back(ts.back());
      }
    }

    return std::accumulate(
      begin(tokens), end(tokens), std::wstring(),
      [](std::wstring str, const std::wstring& t) -> std::wstring {
        return str + L" " + t;
      });
  }

private:
  using Token = std::wstring;
  using Tokens = std::vector<Token>;

  struct Entry {
    struct Reference {
      unsigned count;
    };
    using References = std::map<Token, Reference>;

    References refs;
    unsigned refSum;
  };
  using MarkovStates = std::map<Tokens, Entry>;

  MarkovStates states;
  mutable std::default_random_engine generator;

  Tokens first() const {
    using Diff = Forger::MarkovStates::difference_type;
    std::uniform_int_distribution<Diff> distribution(
      static_cast<Diff>(0),
      static_cast<Diff>(states.size()) - 1);

    // select a start state at random
    auto state = std::next(begin(states), distribution(generator));

    auto tokens = state->first;
    // add a random follow-up state token
    tokens.push_back(randomReference(state));
    return tokens;
  }

  Tokens next(const Tokens& curr) const {
    // find the follow-up state referenced by the last elements of size=order
    auto keys = Tokens(std::next(begin(curr)), end(curr));
    auto state = states.find(keys);

    if(state != end(states)) {
      // add a random follow-up state token
      keys.push_back(randomReference(state));
      return keys;
    } else {
      // these were the final words of an input -> start again fresh
      return first();
    }
  }

  Token randomReference(MarkovStates::const_iterator state) const {
    std::uniform_int_distribution<unsigned> distribution(
      static_cast<unsigned>(0),
      static_cast<unsigned>(state->second.refSum - 1));

    auto random = distribution(generator);

    // select one of the references weighted by their occurence count
    unsigned cdf = 0;
    for(auto&& ref : state->second.refs) {
      cdf += ref.second.count;
      if(random < cdf) {
        return ref.first;
      }
    }
    throw std::logic_error(
      "unexpected random "
      + std::to_string(random)
      + " of " + std::to_string(state->second.refSum));
  }
};

struct Builder {
  void train(std::wistream& is, unsigned order) {
    if(order < 1) {
      throw std::invalid_argument("invalid markov order");
    }

    // split input into separate words
    auto tokens = tokenize(is);

    // shift a window of size=order+1 over the list of tokens
    for(auto front = begin(tokens), back = front + order; back != end(tokens); ++front, ++back) {
      // the window part of size=order are the keys (markov state content)
      auto keys = Forger::Tokens(front, back);
      // the final window element serves as reference to the next markov state
      auto&& value = *back;

      auto state = storage.states.find(keys);
      if(state == end(storage.states)) {
        state = storage.states.emplace(keys, Forger::Entry()).first;
      }

      // insert reference / increment reference count
      state->second.refs[value].count++;
    }
  }

  Forger get() {
    finalize();

    auto ret = std::move(storage);
    storage = Forger();
    return ret;
  }

private:
  enum CharType {
    Space
  , Letter
  , Number
  , Special
  };

  static CharType getType(wchar_t c) {
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

  static void dump(const Forger::Tokens& tokens) {
    std::cout << "Tokens:\n";
    for(auto&& t : tokens) {
      std::wcout << L" " << t << L"\n";
    }
  }

  static Forger::Tokens tokenize(std::wistream& is) {
    Forger::Tokens tokens;
    tokens.reserve(999);
    auto push = [&](Forger::Token& token) {
      if(!token.empty()) {
        tokens.push_back(std::move(token));
        token = Forger::Token();
      }
    };

    Forger::Token token;

    using iterator = std::istream_iterator<wchar_t, wchar_t>;
    for(iterator c(is); c != iterator(); ++c) {
      auto charType = getType(*c);
      if(charType == Space) {
        push(token);
      } else {
        token.push_back(*c);
      }
    }

    push(token);

#ifdef DEBUG_TOKENIZE
    dump(tokens);
#endif

    return tokens;
  }

  void dump() const {
    std::cout << "Trained markov process:\n";
    for(auto&& state : storage.states) {
      for(auto&& t : state.first) {
        std::wcout << L" " << t;
      }
      std::cout << ": ";
      for(auto&& ref : state.second.refs) {
        std::wcout << ref.first
          << L" (" << ref.second.count << L") ";
      }
      std::cout << '\n';
    }
  }

  void finalize() {
    unsigned overall = 0;
    for(auto&& p : storage.states) {
      auto refSum = std::accumulate(
        begin(p.second.refs), end(p.second.refs), unsigned(0),
        [](unsigned sum, const Forger::Entry::References::value_type& p) -> unsigned {
          return sum + p.second.count;
        });
        p.second.refSum = refSum;
        overall += refSum;
    }

#ifdef DEBUG_TRAINING
    dump();
#endif

    std::cout << "Trained with "
      << storage.states.size() << " states with "
      << overall << " references\n\n";
  }

  Forger storage;
};

int main(int argc, char** argv) {
  // set global locale
  std::setlocale(LC_ALL, "de_DE.UTF-8");

  if(argc < 2) {
    return EXIT_FAILURE;
  } else {
    // train word map from user input
    Builder builder;
    for(auto arg = argv + 1; arg < argv + argc; ++arg) {
      std::wifstream ifs(*arg);
      if(!ifs.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(*arg));
      }
      ifs >> std::noskipws;
      builder.train(ifs, markovChainOrder);
    }
    auto forger = builder.get();

    // forge iterations based on user input
    for(;;) {
      int wordCount;
      std::wcout << "How many words to generate? - ";

      std::string line;
      std::getline(std::cin, line);
      if(line.empty()) {
        break;
      } else {
        std::stringstream ss(line);
        if (ss >> wordCount) {
          std::wcout << '\n' << forger.generate(static_cast<size_t>(wordCount)) << "\n\n";
        }
      }
    }

    return EXIT_SUCCESS;
  }
}

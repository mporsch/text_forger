#pragma once

#include <cctype>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>

#include "rejecter.h"

struct Builder {
  using Forger = RejecterDecorator;

  void train(ISTREAM& is) {
    if(MARKOV_CHAIN_ORDER < 1) {
      throw std::invalid_argument("invalid markov order");
    }

    // split input into separate words
    auto tokens = tokenize(is);
    for(auto&& t : tokens) {
      t.sourceIdx = sourceCounts.size();
    }

    // shift a window of size=order+1 over the list of tokens
    for(auto front = begin(tokens), back = front + MARKOV_CHAIN_ORDER; back != end(tokens); ++front, ++back) {
      // the window part of size=order are the keys (markov state content)
      auto keys = Tokens(front, back);
      // the final window element serves as reference to the next markov state
      auto&& value = *back;

      // find/insert state
      auto state = states.find(keys);
      if(state == end(states)) {
        state = states.emplace(std::move(keys), Forger::Transitions()).first;
      }

      // insert reference / increment reference count
      state->second.refs[value].count++;
    }

    sourceCounts.push_back(states.size());
  }

  Forger get() {
    finalize();

    auto forger = ForgerBase(std::move(states));
    auto rejecter = RejecterDecorator(std::move(forger), std::move(sourceCounts));

    *this = Builder();

    return rejecter;
  }

private:
  enum CharType {
    Space
  , Letter
  , Number
  , Special
  };

  static CharType getType(STRING::value_type c) {
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

  static void dump(const Tokens& tokens) {
    std::cout << "Tokens:\n";
    for(auto&& t : tokens) {
      COUT << LITERAL(" ") << t << LITERAL("\n");
    }
  }

  static Tokens tokenize(ISTREAM& is) {
    Tokens tokens;
    tokens.reserve(999);
    auto push = [&](Token& token) {
      if(!token.empty()) {
        tokens.push_back(std::move(token));
        token = Token();
      }
    };

    Token token;

    using iterator = std::istream_iterator<STRING::value_type, STRING::value_type>;
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
    for(auto&& [keys, transitions] : states) {
      for(auto&& token : keys) {
        COUT << LITERAL(" ") << token;
      }
      std::cout << ": ";
      for(auto&& ref : transitions.refs) {
        COUT << ref.first
          << LITERAL(" (") << ref.second.count << LITERAL(") ");
      }
      COUT << Color::Reset() << '\n';
    }
  }

  void finalize() {
    if(sourceCounts.empty()) {
      throw std::runtime_error("untrained");
    }

    unsigned overall = 0;
    for(auto&& [_, transitions] : states) {
      auto refSum = std::accumulate(
        begin(transitions.refs), end(transitions.refs), unsigned(0),
        [](unsigned sum, const Forger::Transitions::References::value_type& p) -> unsigned {
          return sum + p.second.count;
        });
      transitions.refSum = refSum;
      overall += refSum;
    }

    for(auto lhs = begin(sourceCounts), rhs = std::next(lhs); rhs != end(sourceCounts); ++lhs, ++rhs) {
      *rhs -= *lhs;
    }

#ifdef DEBUG_TRAINING
    dump();
#endif

    std::cout << "Trained with "
      << states.size() << " states with "
      << overall << " references, "
      << "from " << sourceCounts
      << "\n\n";
  }

  ForgerBase::MarkovStates states; ///< trained states to be passed to the forger
  RejecterDecorator::SourceCounts sourceCounts; ///< stats to be passed to the decorator
};

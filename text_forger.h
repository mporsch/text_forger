#pragma once

#include <cctype>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "color.h"
#include "literal.h"

struct Token : public STRING {
  friend OSTREAM& operator<<(OSTREAM& os, const Token& token);

  Color color;
};

OSTREAM& operator<<(OSTREAM& os, const Token& token)
{
  os << token.color << static_cast<const STRING&>(token);
  return os;
}


using Tokens = std::vector<Token>;

OSTREAM& operator<<(OSTREAM& os, const Tokens& tokens)
{
  // concatenate tokens separated by space
  auto sep = LITERAL("");
  for(auto &&t : tokens) {
    os << sep << t;
    sep = LITERAL(" ");
  }
  os << Color::Reset();
  return os;
}


struct Forger {
  friend struct Builder;

  Tokens operator()(size_t count) const {
    Tokens tokens;
    tokens.reserve(count);

    // initially add order+1 tokens
    auto ts = first();
    tokens.insert(end(tokens), begin(ts), end(ts));

    // subsequently add only the follow-up state token
    while(tokens.size() < count) {
      ts = next(ts);
      tokens.push_back(ts.back());
    }

    return tokens;
  }

private:
  struct Entry {
    struct Reference {
      unsigned count; ///< number of times this follow-up has been found in training data
    };
    using References = std::map<Token, Reference>;

    References refs; ///< possible follow-ups of this state
    unsigned refSum; ///< sum of follow-up occurrence counts for CDF
  };
  using MarkovStates = std::map<Tokens, Entry>;

  Forger(MarkovStates states)
    : states(std::move(states)) {
  }

  Tokens first() const {
    using Diff = Forger::MarkovStates::difference_type;

    // select a start state at random
    std::uniform_int_distribution<Diff> distribution(
      static_cast<Diff>(0),
      static_cast<Diff>(states.size()) - 1);
    auto state = std::next(begin(states), distribution(generator));
    auto tokens = state->first;

    // add a random follow-up state token
    tokens.push_back(followUp(state));

    return tokens;
  }

  Tokens next(const Tokens& curr) const {
    // find the follow-up state referenced by the last elements of size=order
    auto keys = Tokens(std::next(begin(curr)), end(curr));
    auto state = states.find(keys);

    if(state != end(states)) {
      // add a random follow-up state token
      keys.push_back(followUp(state));
      return keys;
    } else {
      // these were the final words of an input -> start again fresh
      return first();
    }
  }

  Token followUp(MarkovStates::const_iterator state) const {
    std::uniform_int_distribution<unsigned> distribution(
      static_cast<unsigned>(0),
      static_cast<unsigned>(state->second.refSum - 1));
    auto random = distribution(generator);

    // select one of the references weighted by their occurence count
    // using a cumulative distribution function
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

  MarkovStates states;
  mutable std::default_random_engine generator;
};


struct Builder {
  void train(ISTREAM& is, unsigned order) {
    if(order < 1) {
      throw std::invalid_argument("invalid markov order");
    }

    // split input into separate words
    auto tokens = tokenize(is);
    for(auto&& t : tokens)
      t.color = Color::FromId(id);
    ++id;

    // shift a window of size=order+1 over the list of tokens
    for(auto front = begin(tokens), back = front + order; back != end(tokens); ++front, ++back) {
      // the window part of size=order are the keys (markov state content)
      auto keys = Tokens(front, back);
      // the final window element serves as reference to the next markov state
      auto&& value = *back;

      // find/insert state
      auto state = states.find(keys);
      if(state == end(states)) {
        state = states.emplace(keys, Forger::Entry()).first;
      }

      // insert reference / increment reference count
      state->second.refs[value].count++;
    }
  }

  Forger get() {
    finalize();

    auto ret = Forger(std::move(states));
    states = Forger::MarkovStates();
    return ret;
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
    for(auto&& state : states) {
      for(auto&& t : state.first) {
        COUT << LITERAL(" ") << t;
      }
      std::cout << ": ";
      for(auto&& ref : state.second.refs) {
        COUT << ref.first
          << LITERAL(" (") << ref.second.count << LITERAL(") ");
      }
      COUT << Color::Reset() << '\n';
    }
  }

  void finalize() {
    unsigned overall = 0;
    for(auto&& p : states) {
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
      << states.size() << " states with "
      << overall << " references\n\n";
  }

  unsigned int id = 0;
  Forger::MarkovStates states; ///< trained states to be passed to the forger
};

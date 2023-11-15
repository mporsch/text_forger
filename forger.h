#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <vector>

#include "color.h"
#include "literal.h"

struct Token : public STRING {
  friend OSTREAM& operator<<(OSTREAM& os, const Token& token);

  size_t sourceIdx;
};

OSTREAM& operator<<(OSTREAM& os, const Token& token) {
  os << Color::FromId(token.sourceIdx) << static_cast<const STRING&>(token);
  return os;
}


using Tokens = std::vector<Token>;

OSTREAM& operator<<(OSTREAM& os, const Tokens& tokens) {
  // concatenate tokens separated by space
  auto sep = LITERAL("");
  for(auto &&t : tokens) {
    os << sep << t;
    sep = LITERAL(" ");
  }
  os << Color::Reset();
  return os;
}

struct Builder;

struct ForgerBase {
  friend struct Builder;

  ForgerBase(const ForgerBase&) = default;
  ForgerBase(ForgerBase&&) = default;
  virtual ~ForgerBase() = default;
  ForgerBase& operator=(const ForgerBase&) = default;
  ForgerBase& operator=(ForgerBase&&) = default;

  virtual Tokens operator()(size_t count) const {
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
  struct Transitions {
    struct Reference {
      unsigned count = 0; ///< number of times this follow-up has been found in training data
    };
    using References = std::map<Token, Reference>;

    References refs; ///< possible follow-ups of this state
    unsigned refSum = 0; ///< sum of follow-up occurrence counts for CDF
  };
  using MarkovStates = std::map<Tokens, Transitions>;

  ForgerBase(MarkovStates states)
    : states(std::move(states)) {
  }

  Tokens first() const {
    using Diff = ForgerBase::MarkovStates::difference_type;

    // select a start state at random
    std::uniform_int_distribution<Diff> distribution(
      static_cast<Diff>(0),
      static_cast<Diff>(states.size()) - 1);
    auto randomStateIndex = distribution(generator);
    auto state = std::next(begin(states), randomStateIndex);
    auto&& [keys, transitions] = *state;

    // size=order tokens from the start state
    Tokens tokens;
    tokens.reserve(MARKOV_CHAIN_ORDER + 1);
    assert(keys.size() == MARKOV_CHAIN_ORDER);
    tokens = keys;

    // add a random follow-up state token
    tokens.push_back(followUp(transitions));

    return tokens;
  }

  Tokens next(const Tokens& curr) const {
    // find the follow-up state referenced by the last elements of size=order
    Tokens keys;
    keys.reserve(MARKOV_CHAIN_ORDER + 1);
    assert(curr.size() == MARKOV_CHAIN_ORDER + 1);
    keys = Tokens(std::next(begin(curr)), end(curr));
    auto state = states.find(keys);

    if(state != end(states)) {
      // add a random follow-up state token
      keys.push_back(followUp(state->second));
      return keys;
    } else {
      // these were the final words of an input -> start again fresh
      return first();
    }
  }

  Token followUp(const Transitions& transitions) const {
    std::uniform_int_distribution<unsigned> distribution(
      static_cast<unsigned>(0),
      static_cast<unsigned>(transitions.refSum - 1));
    auto randomReferenceIndex = distribution(generator);

    // select one of the references weighted by their occurence count
    // using a cumulative distribution function
    unsigned cdf = 0;
    for(auto&& ref : transitions.refs) {
      cdf += ref.second.count;
      if(randomReferenceIndex < cdf) {
        return ref.first;
      }
    }

    throw std::logic_error(
      "unexpected random "
      + std::to_string(randomReferenceIndex)
      + " of " + std::to_string(transitions.refSum));
  }

protected:
  MarkovStates states;
  mutable std::default_random_engine generator;
};

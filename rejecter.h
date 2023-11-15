#pragma once

#include "forger.h"

#include <cassert>
#include <iostream>

struct Builder;

struct RejecterDecorator : public ForgerBase {
  friend struct Builder;

  using SourceCounts = std::vector<size_t>;

  RejecterDecorator(ForgerBase&& f, SourceCounts sourceCounts)
      : ForgerBase(std::move(f))
      , sourceCounts(std::move(sourceCounts)) {
  }

  RejecterDecorator(const RejecterDecorator&) = default;
  RejecterDecorator(RejecterDecorator&&) = default;
  ~RejecterDecorator() override = default;
  RejecterDecorator& operator=(const RejecterDecorator&) = default;
  RejecterDecorator& operator=(RejecterDecorator&&) = default;

  Tokens operator()(size_t count) const override {
    Tokens tokens;
    do {
      tokens = ForgerBase::operator()(count);
    } while(!isBalanced(tokens));
    return tokens;
  }

private:
  bool isBalanced(const Tokens& tokens) const {
    decltype(sourceCounts) tokenSourceCounts(sourceCounts.size(), 0);
    for(auto&& t : tokens) {
      assert(t.sourceIdx < tokenSourceCounts.size());
      ++tokenSourceCounts[t.sourceIdx];
    }

    for(size_t i = 0; i < sourceCounts.size(); ++i) {
      auto sourceRatio = static_cast<double>(sourceCounts[i]) / this->states.size();
      auto tokenRatio = static_cast<double>(tokenSourceCounts[i]) / tokens.size();
      if(!isBalanced(sourceRatio, tokenRatio)) {
#ifdef DEBUG_REJECT
        COUT << "rejecting: \n\t" << tokens << "\n\n";
#endif // DEBUG_REJECT

        return false;
      }
    }
    return true;
  }

  static bool isBalanced(double sourceRatio, double tokenRatio) {
    return (sourceRatio < tokenRatio * 2);
  }

  SourceCounts sourceCounts;
};

std::ostream& operator<<(std::ostream& os, const RejecterDecorator::SourceCounts& sc) {
  os << sc.size() << " sources with ";
  auto sep = "";
  for(auto&& s : sc) {
    os << sep << s;
    sep = "/";
  }
  os << " states";
  return os;
}

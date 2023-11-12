cmake_minimum_required(VERSION 3.1.0)

project(text_forger CXX)

option(DEBUG_TOKENIZE "" FALSE)
option(DEBUG_TRAINING "" FALSE)
option(USE_WSTRING "" FALSE)
option(USE_COLOR "" FALSE)
set(MARKOV_CHAIN_ORDER 2 CACHE STRING "a higher chain order causes more coherent output but reduces variation")

add_executable(text_forger
  main.cpp
  color.h
  literal.h
  text_forger.h
)
target_compile_definitions(text_forger PRIVATE
  $<$<BOOL:${DEBUG_TOKENIZE}>:DEBUG_TOKENIZE>
  $<$<BOOL:${DEBUG_TRAINING}>:DEBUG_TRAINING>
  $<$<BOOL:${USE_WSTRING}>:USE_WSTRING>
  $<$<BOOL:${USE_COLOR}>:USE_COLOR>
  MARKOV_CHAIN_ORDER=${MARKOV_CHAIN_ORDER}
)
if(USE_WSTRING)
  target_compile_definitions(text_forger PRIVATE
    STRING=std::wstring
    OSTREAM=std::wostream
    COUT=std::wcout
    ISTREAM=std::wistream
    IFSTREAM=std::wifstream
    OSTRINGSTREAM=std::wostringstream
  )
else()
  target_compile_definitions(text_forger PRIVATE
    STRING=std::string
    OSTREAM=std::ostream
    COUT=std::cout
    ISTREAM=std::istream
    IFSTREAM=std::ifstream
    OSTRINGSTREAM=std::ostringstream
  )
endif()

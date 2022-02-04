//
// Created by Nathaniel Rupprecht on 11/27/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_UTILITY_H_
#define GEMINI_INCLUDE_GEMINI_UTILITY_H_

#include "gemini/export.hpp"
#include <sstream>
#include <string>
#include <vector>

///========================================================================
/// Macros for utility and error handling.
///========================================================================

//! \brief Pretty version of [[nodiscard]]
#define NO_DISCARD [[nodiscard]]

#define GEMINI_FAIL(message) if (true) {   \
    std::stringstream mess;                \
    mess << message;                       \
    throw std::runtime_error(mess.str());  \
  } else

#define GEMINI_REQUIRE(condition, message) if (!(condition)) {   \
    std::stringstream mess;                \
    mess << message;                       \
    throw std::runtime_error(mess.str());  \
  } else

#define GEMINI_ASSERT(condition, message) if (!(condition)) {   \
    std::stringstream mess;                \
    mess << message;                       \
    throw std::runtime_error(mess.str());  \
  } else

///========================================================================
/// Math functions.
///========================================================================

namespace gemini::math {

template<typename T>
inline GEMINI_EXPORT T square(const T &x) {
  return x * x;
}

constexpr double PI = 3.14159265358979323;

} // namespace gemini::math

#endif //GEMINI_INCLUDE_GEMINI_UTILITY_H_

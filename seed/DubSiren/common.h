#pragma once
#ifndef COMMON_H_
#define COMMON_H_

#include "constants.h"

struct CoordinateToHash {
  float x;
  float y;
  unsigned long hash;
};

#endif  // COMMON_H_
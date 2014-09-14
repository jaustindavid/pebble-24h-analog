#pragma once
#include "pebble.h"

static const GPathInfo HOUR_HAND_POINTS = {
  4,
  (GPoint []) {
    {-8,  0 },
    { 0,  16},
    { 8,  0 },
    { 0, -64 }
  }
};

#define CENTER_X 71
#define BORDER_Y 167
static const GPathInfo MIDNIGHT_POINTS = {
  3,
  (GPoint []) {
    {CENTER_X - 5, 4},
    {CENTER_X + 5, 4},
    {CENTER_X,    20},
  }
};

static const GPathInfo NOON_POINTS = {
  3,
  (GPoint []) {
    {CENTER_X - 5, BORDER_Y - 4},
    {CENTER_X + 5, BORDER_Y - 4},
    {CENTER_X, BORDER_Y - 20},
  }
};


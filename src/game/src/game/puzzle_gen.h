#ifndef PUZZLE_GEN_H
#define PUZZLE_GEN_H

#include "sokoban.h"

// Generate a random solvable puzzle
// difficulty: 0=easy (1 box), 1=medium (2 boxes), 2=hard (3 boxes)
// seed: random seed for generation
// Returns true if generation succeeded
bool puzzle_generate(GameState& gs, int difficulty, int seed);

#endif // PUZZLE_GEN_H

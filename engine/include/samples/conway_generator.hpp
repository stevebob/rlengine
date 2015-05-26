#ifndef _CONWAY_GENERATOR_HPP_
#define _CONWAY_GENERATOR_HPP_

#include "world/generator.hpp"
#include "samples/game_cell.hpp"
#include "character/character.hpp"

class conway_generator : public generator<character, game_cell> {
    
    private:
    class conway_cell;
    class conway_grid;
    int generate_attempt(conway_grid &cg);

    public:
    void generate(world<character, game_cell> &w);
};

#endif

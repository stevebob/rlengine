#ifndef _WORLD_HPP_
#define _WORLD_HPP_

#include "world/grid.hpp"
#include "world/world_cell.hpp"
#include "geometry/vec2.hpp"
#include "character/character.hpp"
#include "transaction/transaction_queue.hpp"
#include <vector>
#include <memory>

template <typename C, typename W, typename T> class world {
    public:
    std::vector<grid<world_cell>> maps;
    std::vector<std::unique_ptr<character>> characters;
    typedef transaction_queue<transaction<T, C, W>, character, world_cell> transaction_queue_t;
    transaction_queue_t transactions;
    typedef typename transaction_queue_t::transaction_t transaction_t;
    typedef typename transaction_queue_t::transaction_ptr_t transaction_ptr_t;

    const int width;
    const int height;
    world(const int width, const int height) : 
        width(width),
        height(height)
    {
        maps.push_back(grid<world_cell>(width, height));
    }

    void with_character_at_coord(int level, vec2<int> coord, const std::function<void(character&)> &f) {
        std::for_each(characters.begin(), characters.end(), [&](std::unique_ptr<character> &c) {
            if (c->level_index == level && c->coord == coord) {
                f(*c);
            }
        });
    }

    world_cell& get_random_empty_cell(int index) {
        for (;;) {
            int x = rand() % width;
            int y = rand() % height;
            world_cell& ret = maps[index].get_cell(x, y);
            if (!ret.is_solid()) {
                return ret;
            }
        }
        return maps[index][0][0];
    }

    void move_character(character &c, vec2<int> coord) {
        if (!maps[c.level_index].get_cell(coord).is_solid()) {
            c.coord = coord;
        }
    }

};

#endif

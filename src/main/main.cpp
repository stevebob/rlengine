#include "world/world.hpp"
#include "actor/character_actor.hpp"
#include "control/control.hpp"
#include "control/curses_control.hpp"
#include "world/conway_generator.hpp"
#include "world/border_generator.hpp"
#include "observer/shadow_cast_fov.hpp"
#include "actor/always_move_left.hpp"
#include "drawing/curses_drawer.hpp"

#include "debug/fifo.hpp"
#include "io/curses.hpp"

#include "util/cancellable.hpp"

#include "util/perlin.hpp"
#include "util/arith.hpp"
#include "assert.h"
#include <memory>

class basic_character : public character {
    public:
    basic_character(world &w, const vec2<int> &v) :
        character(w, v, 'b', PAIR_RED, PAIR_DARK_RED, 10)
    {}
};

class test {
    bool is_cancelled() {return false;}
};

void f(int low, int high, int r, int g, int b) {
    for (int i = low; i < high; ++i) {
        init_color(20 + i, r, g, b);
    }
}

class test_cell : public cell {
    public:
    test_cell(const int j, const int i) : cell(j, i) {};
    int data;
    char ch = ' ';
    double noise;
    int generation = -1;
    test_cell *core = nullptr;
};

test_cell& get_min_border(grid<test_cell> &grid) {
    test_cell *x = nullptr;
    double min_noise = 1;
    grid.for_each_border([&](test_cell &c) {
        if (x == nullptr || c.noise < min_noise) {
            x = &c;
            min_noise = c.noise;
        }
    });

    return *x;
}

class search_cell : public cell {
    public:
    search_cell(const int j, const int i) : cell(j, i) {};
    search_cell *parent = nullptr;
    bool visited = false;
};

class search_node {
    public:
    test_cell *tc;
    double cost;
    int dist;
    search_node(test_cell &tc, double cost, int dist) :
        tc(&tc),
        cost(cost),
        dist(dist)
    {
    }
};

class search_node_comparitor {
    public:
    bool operator()(const search_node &a, const search_node &b) {
        return a.cost > b.cost;
    }
};

bool dijkstra(grid<test_cell> &gr) {
    simple_grid<search_cell> search_grid(gr.width, gr.height);
    test_cell *start = &get_min_border(gr);
    test_cell *end = nullptr;
    std::priority_queue<search_node, std::vector<search_node>, search_node_comparitor> pq;
    pq.push(search_node(*start, 0, 0));
    search_grid.get_cell(start->coord).visited = true;
    
    std::list<test_cell*> ends;

    while (!pq.empty()) {
        search_node n = pq.top();
        pq.pop();
        test_cell *current = n.tc;
        double cost = n.cost;
        int dist = n.dist;

        if (gr.is_border_cell(*current) && dist > 100) {
            if (end == nullptr) {
                end = current;
    
                search_cell *tmp = &search_grid.get_cell(end->coord);
                while (tmp) {
                    gr.get_cell(tmp->coord).data = COLOR_BLUE; //COLOR_RED;
                    gr.get_cell(tmp->coord).generation = 0;
                    gr.get_cell(tmp->coord).core = &gr.get_cell(tmp->coord);
                    tmp = tmp->parent;
                }

            } else {
                if (arithmetic::random_double(0, 1) < 0.01) {
                    ends.push_back(current);
                }
            }
        }

        gr.for_each_neighbour(*current, [&](test_cell &nei) {
            search_cell &sc = search_grid.get_cell(nei.coord);
            if (!sc.visited) {
                sc.parent = &search_grid.get_cell(current->coord);
                sc.visited = true;
                double diff =  (nei.noise - current->noise);
                pq.emplace(nei, cost + diff, dist + 1);
            }
        });

    }

    std::for_each(ends.begin(), ends.end(), [&](test_cell *cptr) {
        search_cell *tmp = &search_grid.get_cell(cptr->coord);
        while (tmp) {
            if (gr.get_cell(tmp->coord).generation != -1) {
                break;
            }
            gr.get_cell(tmp->coord).data = COLOR_BLUE; //COLOR_RED;
            gr.get_cell(tmp->coord).generation = 3;
            gr.get_cell(tmp->coord).core = &gr.get_cell(tmp->coord);
            tmp = tmp->parent;
        }   
    });

    return true;
}

void grow(grid<test_cell> &gr, int gen) {
    gr.for_each([&](test_cell &c) {
        if (c.generation == gen) {
            gr.for_each_cardinal_neighbour(c, [&](test_cell &nei) {
                if (nei.generation == -1) {
                    test_cell *core = c.core;
                    int next_gen = gen + 1;
                    if (abs(nei.noise - core->noise) < (next_gen * 0.01)) {
                        nei.generation = next_gen;
                        nei.core = core;
                        nei.data = COLOR_BLUE;//COLOR_RED + next_gen;
                    }
                }
            });
        }
    });
}

void greedy(grid<test_cell> &gr) {

    grid<generic_cell<bool>> visited(gr.width, gr.height);

    visited.for_each([](generic_cell<bool> &c) {c.data = false;});
    test_cell &c = get_min_border(gr);
    test_cell *current = &c;

    while (true) {
        current->data = COLOR_RED;
        current->ch = 'x';
        visited.get_cell(current->coord).data = true;
        test_cell *next = nullptr;
        double min_noise = 1;
        gr.for_each_neighbour(*current, [&](test_cell &n) {
            if (!visited.get_cell(n.coord).data && (next == nullptr || n.noise < min_noise)) {
                next = &n;
                min_noise = n.noise;
            }
        });
        if (next == nullptr) {
            break;
        } else {
            current = next;
        }
    }
}


int main(int argc, char *argv[]) {
    srand(time(NULL));
//    srand(0);
#if 0
    std::list<std::unique_ptr<active_effect>> test_list;
    cancellable_owner_list<active_effect> test_list_2;
    cancellable_owner_list<active_effect> test_list_3;

    test_list_2 = test_list_3;

    fifo::start();
    curses::simple_start();

    world w(100, 40);
    std::make_unique<conway_generator>()->generate(w);

    shadow_cast_fov fov;
    curses_drawer dr;

    w.characters.push_back(std::make_unique<character>(w, w.get_random_empty_cell(0).coord));
    w.characters.push_back(std::make_unique<basic_character>(w, w.get_random_empty_cell(0).coord));
    w.characters.push_back(std::make_unique<basic_character>(w, w.get_random_empty_cell(0).coord));
    w.characters.push_back(std::make_unique<basic_character>(w, w.get_random_empty_cell(0).coord));
    w.characters.push_back(std::make_unique<basic_character>(w, w.get_random_empty_cell(0).coord));

    curses_control a0(*w.characters[0], fov, dr);
    always_move_left a1(*w.characters[1], fov);
    always_move_left a2(*w.characters[2], fov);
    always_move_left a3(*w.characters[3], fov);
    always_move_left a4(*w.characters[4], fov);

    a0.init_dvorak();

    w.get_schedule().register_callback(a0, 0);
    w.get_schedule().register_callback(a1, 1);
    w.get_schedule().register_callback(a2, 2);
    w.get_schedule().register_callback(a3, 3);
    w.get_schedule().register_callback(a4, 4);

    w.get_schedule().run_until_empty();

    curses::simple_stop();
    fifo::stop();
#endif
#define DRAW
#ifdef DRAW
    char envstr[] = "TERM=xterm-256color";
    putenv(envstr);

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();

    const int offset = 20;
    for (int i = 0; i < 200; ++i) {
        init_color(offset + i, i*5, i*5, i*5);
        init_pair(offset + i, 0, offset + i);
    }
    for (int i = 0; i < 20; ++i) {
        init_pair(i, 0, i);
    }
#endif
    perlin_grid pg;
    grid<test_cell> dg(220, 60);
    
    dg.for_each([&](test_cell &c) {
        c.noise = pg.get_noise(c.centre * 0.1);
        c.data = (c.noise + 1) * 100;
    });
 
 
    dijkstra(dg);
    grow(dg, 0);
    grow(dg, 1);
    grow(dg, 2);
    grow(dg, 3);

#ifdef DRAW
    dg.for_each([&](test_cell &c) {
        wmove(stdscr, c.y_coord, c.x_coord);
        wattron(stdscr, COLOR_PAIR(c.data));
        waddch(stdscr, c.ch);
        wattroff(stdscr, COLOR_PAIR(c.data));

    });

    wgetch(stdscr);

    endwin();
#endif
    return 0;
}

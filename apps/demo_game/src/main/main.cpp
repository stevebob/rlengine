#include <world/world.hpp>
#include <samples/conway_generator.hpp>
#include <io/curses.hpp>
#include <samples/curses_drawer.hpp>
#include <actor/player_actor.hpp>
#include <ncurses.h>
#include <observer/shadow_cast_fov.hpp>

int main(int argc, char *argv[]) {
    curses::simple_start();
    srand(2);

    world<character, game_cell> w(100, 40);
    conway_generator gen;
    gen.generate(w);
    shadow_cast_fov<character, game_cell, knowledge_cell> fov;
    curses_drawer dr;
    schedule<world<character, game_cell>> s;
    character player(w.get_random_empty_cell(0).coord);
    player_actor<character, game_cell, knowledge_cell>  actor(player, w, fov, dr);
    actor.init_dvorak();


    s.register_callback(actor, 0);


    s.run_until_empty(w);
    
    curses::simple_stop();

    return 0;
}

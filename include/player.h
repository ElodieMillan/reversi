#ifndef PLAYER_H
#define PLAYER_H

#include <time.h>
#include <unistd.h>

#include <board.h>


/***************************** Intern management ******************************/

/* To activate verbose mode. */
void set_verbose (void);


/********************************* Heuristics *********************************/

/* A player function that returns a
 * chosen move depending on the given board */
move_t human_player (board_t *board);

/* A random AI player that return a random move. */
move_t random_player (board_t *board);

/* A minimax AI player that return a move compute with minimax algorithm. */
move_t minimax_player (board_t *board);

/* A minimax AI player that return a move compute with minimax algorithm and
 * use alpha and beta parameter to cut some branch on the tree. */
move_t minimax_ab_player (board_t *board);

/* A minimax AI player that return a move compute with minimax algorithm and
 * use alpha and beta parameter to cut some branch on the tree.
 * Some interesting move (like corner) are weighed to choose (or not)
 * these moves. */
move_t newton_player (board_t *board);


#endif /* PLAYER_H */

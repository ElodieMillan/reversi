#ifndef BOARD_H
#define BOARD_H

/* Min/Max width board. */
#define MIN_BOARD_SIZE 2
#define MAX_BOARD_SIZE 10

/* Possibles directions. */
#define DIRECTIONS 8

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/********************************* Structures *********************************/

/* Board discs. */
typedef enum
{
    BLACK_DISC = 'X',
    WHITE_DISC = 'O',
    EMPTY_DISC = '_',
    HINT_DISC = '*',
} disc_t;

/* A move in the reversi game. */
typedef struct
{
    size_t row;
    size_t column;
} move_t;

/* Store the score of the game. */
typedef struct
{
    unsigned short black;
    unsigned short white;
} score_t;

/* Reversi board (forward declaration to hide the implementation). */
typedef struct board_t board_t;


/*************************** bitboard_t management ****************************/

/* Base bitboard type. */
typedef unsigned __int128 bitboard_t;

/* A SWAR popcount alorithm for bitboard_t. */
size_t bitboard_popcount (const bitboard_t bitboard);


/***************************** board_t management *****************************/

/* --------------------------- General management --------------------------- */

/* Check if the given size is valid for a board_t. */
bool board_cheak_size (const size_t size);

/* Get the size of the board 'board'. */
size_t board_size (const board_t *board);

/* Get the current player of the board 'board'. */
disc_t board_player (const board_t *board);

/* Set the current player. */
void board_set_player (board_t *board, disc_t player);

/* Get the content of the square at the given coordinate. */
disc_t board_get (const board_t *board, const size_t row, const size_t column);

/* Set the content of the square at the given coordinate. */
void board_set (board_t *board, const disc_t disc, const size_t row,
                const size_t column);

/* Get the score of the given board. */
score_t board_score (const board_t *board);

/* Write on the file descriptor fd the content of the given board
 *   -> return number printed caracter or '-1' on error. */
int board_print (const board_t *board, FILE *fd);

/* ---------------- Memory allocation management for boart_t ---------------- */

/* Allocate memory to create board of size 'size'
 *   -> return pointer to this newly board
 *      or NULL if something wrong appended. */
board_t *board_alloc (const size_t size, const disc_t player);

/* Free the memory allocated to the board 'game_board'. */
void board_free (board_t *game_board);

/* Initialize all the square of the board as a starting game
 * with the size 'size'. */
board_t *board_init (const size_t size);

/* Create a copy of the board structure of 'board'. */
board_t *board_copy (const board_t *board);

/* ---------------------------- Moves management ---------------------------- */

/* Count the number of possible move for the current player. */
size_t board_count_player_moves (const board_t *board);

/* Check if a move is valid within the given board. */
bool board_is_move_valid (const board_t *board, const move_t move);

/* Apply a move according to rules and set the board for next moves
 * --> past to the next player or same player if second player can't move
 * --> or EMPTY_DISC if nobody can move. */
bool board_play (board_t *board, const move_t move);

/* Copy the possible moves on this last bitboard
 * and clear the last bit of the bitboard at each call to the board_next_move ()
 * function. */
move_t board_next_move (board_t *board);


/************************ bitboard_t corner management ************************/

/* Test if the move is a corner. */
bool is_corner (const size_t size, const move_t move);

/* Convert the i-th corner as move_t. */
move_t get_corner_as_move (const size_t size, const short i);

/* For Newton AI, this methode is in charge to compute all moves as
 * corner that player need to protect, as a bitboard. The computation is made
 * thanks to bitboard_t in the aim to make a efficiant code. */
bitboard_t get_corners_to_exam (const board_t *actual_board);


/*********************** bitboard_t borders management ************************/

/* Create an array of bitboards that represent
 * north, south, east and west borders. */
bitboard_t* get_borders (const size_t size);

/* Get the initial postition of the 4ths borders. */
bitboard_t* get_boarders_init (const size_t size);

/* Get the distance between two points of the 4ths borders. */
size_t* get_borders_increment (const size_t size);

/* Transforme a bitboard_t of borders as move_t. */
move_t get_border_as_move (const bitboard_t bit, const size_t size,
                           const short border);

/* Compute all the interesting borders that is safe to do. */
bitboard_t get_interesting_borders (const board_t *board);


#endif /* BOARD_H */

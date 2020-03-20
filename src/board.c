#include <board.h>

#include <string.h>


/********************************* Structure **********************************/

/* Internal board_t structure (hiden from the outsid) */
struct board_t
{
    size_t size;
    disc_t player;
    bitboard_t black;
    bitboard_t white;
    bitboard_t moves;
    bitboard_t next_move;
};


/*************************** Function declarations ****************************/

/* --------------------------------- Shifts --------------------------------- */

static bitboard_t shift_north (const size_t, const bitboard_t);
static bitboard_t shift_ne (const size_t, const bitboard_t);
static bitboard_t shift_east (const size_t, const bitboard_t);
static bitboard_t shift_se (const size_t, const bitboard_t);
static bitboard_t shift_south (const size_t, const bitboard_t);
static bitboard_t shift_sw (const size_t, const bitboard_t);
static bitboard_t shift_west (const size_t, const bitboard_t);
static bitboard_t shift_nw (const size_t, const bitboard_t);

/* ---------------------------- Moves management ---------------------------- */

static bitboard_t compute_moves (const size_t size, const bitboard_t player,
                                 const bitboard_t opponent);


/********************************* Constants **********************************/

/* Arrays used for board_print. */
static const char columns[] = "A B C D E F G H I J";
static const size_t rows[] = {1,2,3,4,5,6,7,8,9,10};

/* Array of function pointer on directions. */
static bitboard_t (*shifts[DIRECTIONS]) (const size_t, const bitboard_t) =
{shift_north, shift_ne, shift_east, shift_se,
 shift_south, shift_sw, shift_west, shift_nw};

/* Movement to do in the row in function of the direction. */
static int row_direction[DIRECTIONS] = {1, 1, 0, -1, -1, -1, 0, 1};

/* Movement to do in the column in function of the direction. */
static int column_direction[DIRECTIONS] = {0, -1, -1, -1, 0, 1, 1, 1};


/*************************** bitboard_t management ****************************/

size_t
bitboard_popcount (const bitboard_t bitboard)
{
    bitboard_t only_3 = 0x3333333333333333;
    bitboard_t only_5 = 0x5555555555555555;
    bitboard_t only_0F = 0x0F0F0F0F0F0F0F0F;
    bitboard_t only_01 = 0x0101010101010101;
    bitboard_t copy1 = bitboard;
    bitboard_t copy2 = bitboard >> 64;

    copy1 -= ((copy1 >> 1) & only_5);
    copy1 = (copy1 & only_3) + ((copy1 >> 2) & only_3);
    copy1 = (copy1 + (copy1 >> 4)) & only_0F;

    copy2 -= ((copy2 >> 1) & only_5);
    copy2 = (copy2 & only_3) + ((copy2 >> 2) & only_3);
    copy2 = (copy2 + (copy2 >> 4)) & only_0F;

    return (((copy1 * only_01) >> 56) & 0x7F) +
           (((copy2 * only_01) >> 56) & 0x7F);
}

/* --------------------------------- Shifts --------------------------------- */

static bitboard_t
shift_north (const size_t size, const bitboard_t bitboard)
{
    if (!board_cheak_size (size))
    {
        return (bitboard_t) 0;
    }

    bitboard_t ones = ((bitboard_t) 1 << (size * (size - 1))) - 1;
    ones = ones << size;
    bitboard_t first_row = bitboard & (~ones);
    bitboard_t bitboard2 = bitboard & ones;

    return ((bitboard2 >> size) | (first_row << ((size - 1) * size)));
}

static bitboard_t
shift_south (const size_t size, const bitboard_t bitboard)
{
    if (!board_cheak_size (size))
    {
        return (bitboard_t) 0;
    }

    bitboard_t ones = ((bitboard_t) 1 << (size * (size - 1))) - 1;
    bitboard_t first_row = bitboard & (~ones);
    bitboard_t bitboard2 = bitboard & ones;

    return ((bitboard2 << size) | (first_row >> ((size - 1) * size)));
}

static bitboard_t
shift_east (const size_t size, const bitboard_t bitboard)
{
    if (!board_cheak_size (size))
    {
        return (bitboard_t) 0;
    }

    bitboard_t ones = ((bitboard_t) 1 << (size - 1)) - 1;

    for (size_t i = 0; i < (size - 1); i++)
    {
        ones |= (ones << size);
    }

    bitboard_t last_column = bitboard & (~ones);
    bitboard_t bitboard2 = bitboard & ones;
    bitboard2 = bitboard2 << 1;
    last_column = last_column >> (size - 1);

    return (bitboard2 | last_column);
}

static bitboard_t
shift_west (const size_t size, const bitboard_t bitboard)
{
    if (!board_cheak_size (size))
    {
        return (bitboard_t) 0;
    }

    bitboard_t ones = ((bitboard_t) 1 << (size - 1)) - 1;
    ones = ones << 1;

    for (size_t i = 0; i < (size - 1); i++)
    {
        ones |= (ones << size);
    }

    bitboard_t last_column = bitboard & (~ones);
    bitboard_t bitboard2 = bitboard & ones;
    bitboard2 = bitboard2 >> 1;
    last_column = last_column << (size - 1);

    return (bitboard2 | last_column);
}

static bitboard_t
shift_nw (const size_t size, const bitboard_t bitboard)
{
    return shift_north (size, shift_west (size, bitboard));
}

static bitboard_t
shift_sw (const size_t size, const bitboard_t bitboard)
{
    return shift_south (size, shift_west (size, bitboard));
}

static bitboard_t
shift_se (const size_t size, const bitboard_t bitboard)
{
    return shift_south (size, shift_east (size, bitboard));
}

static bitboard_t
shift_ne (const size_t size, const bitboard_t bitboard)
{
    return shift_north (size, shift_east (size, bitboard));
}

/* --------------------------- General management --------------------------- */

/* Set at the bit (row, column) to 1 in the returned bitboard that its
 * size is equale to 'size'. */
static bitboard_t
set_bitboard (const size_t size, const size_t row, const size_t column)
{
    if (!board_cheak_size (size) || row >= size || column >= size)
    {
        return (bitboard_t) 0;
    }

    return ((bitboard_t) 1) << (row * size + column);
}


/***************************** board_t management *****************************/

/* --------------------------- General management --------------------------- */

bool
board_cheak_size (const size_t size)
{
    return size >= MIN_BOARD_SIZE && size <= MAX_BOARD_SIZE && (size % 2) == 0;
}

size_t
board_size (const board_t *board)
{
    if (board == NULL)
    {
        return 0;
    }

    return board->size;
}

disc_t
board_player (const board_t *board)
{
    if (board == NULL)
    {
        return EMPTY_DISC;
    }

    return board->player;
}

void
board_set_player (board_t *board, disc_t player)
{
    if (board == NULL || player == HINT_DISC)
    {
        return;
    }

    board->player = player;

    if (board->player == BLACK_DISC)
    {
        board->moves = compute_moves (board->size, board->black, board->white);
    }
    else if (board->player == WHITE_DISC)
    {
        board->moves = compute_moves (board->size, board->white, board->black);
    }
}

disc_t
board_get (const board_t *board, const size_t row, const size_t column)
{
    if (board == NULL || row >= board->size || column >= board->size)
    {
        return EMPTY_DISC;
    }

    bitboard_t bit = set_bitboard (board->size, row, column);

    if ((board->black & bit) != 0)
    {
        return BLACK_DISC;
    }
    else if ((board->white & bit) != 0)
    {
        return WHITE_DISC;
    }
    else if ((board->moves & bit) != 0)
    {
        return HINT_DISC;
    }
    else
    {
        return EMPTY_DISC;
    }
}

void
board_set (board_t *board, const disc_t disc, const size_t row,
           const size_t column)
{
    if (board == NULL || row >= board->size || column >= board->size)
    {
        return;
    }

    bitboard_t bit = set_bitboard (board->size, row, column);

    switch (disc)
    {
        case BLACK_DISC :
            board->black |= bit;
            board->white &= ~bit;

            break;

        case WHITE_DISC :
            board->white |= bit;
            board->black &= ~bit;

            break;

        case EMPTY_DISC :
            board->black &= ~bit;
            board->white &= ~bit;

            break;

        default :
            break;
    }

    if (board->player == EMPTY_DISC)
    {
        return;
    }

    /* Adapt the new possibility to move * with compute_moves (). */
    if (board->player == BLACK_DISC)
    {
        board->moves = compute_moves (board->size, board->black, board->white);
    }
    else
    {
        board->moves = compute_moves (board->size, board->white, board->black);
    }
}

score_t
board_score (const board_t *board)
{
    if (board == NULL)
    {
        return (score_t) {.white = 0, .black = 0};
    }

    return (score_t) {.white = bitboard_popcount (board->white),
                      .black = bitboard_popcount (board->black)};
}

int
board_print (const board_t *board, FILE *fd)
{
    if (board == NULL || fd == NULL)
    {
        return -1;
    }

    int counter = 0;
    score_t the_score = board_score (board);
    char columns_name[board_size (board) * 2 + 1];
    memcpy (columns_name, &columns, board_size (board) * 2);
    columns_name[board_size (board) * 2] = '\0';
    counter += fprintf (fd, "\n'%c' player's turn.\n\n   %s\n",
                        board->player, columns_name);

    for (size_t i = 0; i < board->size; i++)
    {
        if (rows[i] < 10)
        {
            fputc (' ', fd);
        }

        counter += fprintf (fd, "%ld ", rows[i]);

        for (size_t j = 0; j < board->size; j++)
        {
            counter += fprintf (fd, "%c ", board_get (board, i, j));
        }

        fputc ('\n', fd);
    }
    counter += fprintf (fd, "\nScore: 'X' = %d, 'O' = %d.\n\n\n",
                        the_score.black, the_score.white) + 2 * board->size;

    if (board->size > 8)
    {
        counter--;
    }

    return counter;
}

/* ---------------- Memory allocation management for boart_t ---------------- */

board_t*
board_alloc (const size_t size, const disc_t player)
{
    if (!board_cheak_size (size) ||
        (player != BLACK_DISC && player != WHITE_DISC))
    {
        return NULL;
    }

    board_t *game_board = malloc (sizeof (board_t));

    if (game_board == NULL)
    {
        return NULL;
    }

    game_board->size = size;
    game_board->player = player;
    game_board->black = 0;
    game_board->white = 0;
    game_board->moves = 0;
    game_board->next_move = 0;

    return game_board;
}

void
board_free (board_t *game_board)
{
    if (game_board == NULL)
    {
        return;
    }

    free (game_board);
}

board_t*
board_init (const size_t size)
{
    board_t *game_board = board_alloc (size, BLACK_DISC);

    if (game_board == NULL)
    {
        return NULL;
    }

    game_board->white = set_bitboard (size, size / 2 - 1, size / 2 - 1);
    game_board->white |= set_bitboard (size, size / 2, size / 2);
    game_board->black = set_bitboard (size, size / 2 - 1, size / 2);
    game_board->black |= set_bitboard (size, size / 2, size / 2 - 1);
    game_board->moves = compute_moves (size, game_board->black,
                                       game_board->white);

    if (size == 2) /* the game is already finished. */
    {
        game_board->player = EMPTY_DISC;
    }

    return game_board;
}

board_t*
board_copy (const board_t *board)
{
    if (board == NULL)
    {
        return NULL;
    }

    board_t *game_board = board_alloc (board->size, board->player);

    if (game_board == NULL)
    {
        return NULL;
    }

    game_board->black = board->black;
    game_board->white = board->white;
    game_board->moves = board->moves;
    game_board->next_move = board->next_move;

    return game_board;
}

/* ---------------------------- Moves management ---------------------------- */

/* Find the distance between two no-opponents to know the size associate. */
static size_t
board_distance_between_no_opponents (const size_t size,
                                     const bitboard_t opponent,
                                     const move_t move, const size_t direction)
{
    bitboard_t adversary = opponent;
    bitboard_t bit = set_bitboard (size, move.row, move.column);
    size_t gap = 0;

    do
    {
        adversary = shifts[direction] (size, adversary);
        gap++;
    } while ((adversary & bit) != 0);

    return gap;
}

/* Compute all the possible moves as HINT_DISC (*) for the current player.  */
static bitboard_t
compute_moves (const size_t size, const bitboard_t player,
                                  const bitboard_t opponent)
{
    bitboard_t possible_moves = 0;

    for (size_t row = 0; row < size; row++)
    {
        for (size_t col = 0; col < size; col++)
        {
            if ((player & set_bitboard (size, row, col)) == 0)
            {
                continue;
            }

            for (size_t d = 0; d < DIRECTIONS; d++)
            {
                move_t move = (move_t) {.row = row, .column = col};
                size_t gap = board_distance_between_no_opponents (size,
                                                                  opponent,
                                                                  move, d);
                int i = row + row_direction[d] * gap;
                int j = col + column_direction[d] * gap;

                if (gap <= 1 || i < 0 || ((size_t) i) >= size ||
                    j < 0 || ((size_t) j) >= size)
                {
                    continue;
                }

                bitboard_t bit = set_bitboard (size, (size_t) i, (size_t) j);

                if ((player & bit) == 0)
                {
                    possible_moves |= bit;
                }
            }
        }
    }

    return possible_moves;
}

size_t
board_count_player_moves (const board_t *board)
{
    if (board == NULL ||
        ((board->player != WHITE_DISC) && (board->player != BLACK_DISC)))
    {
        return 0;
    }

    return bitboard_popcount (board->moves);
}

bool
board_is_move_valid (const board_t *board, const move_t move)
{
    if (board == NULL ||
        ((board->player != WHITE_DISC) && (board->player != BLACK_DISC)))
    {
        return false;
    }

    return ((set_bitboard (board->size, move.row, move.column) &
             board->moves) != (bitboard_t) 0);
}

/* Reverse all the opponent between two player disc. */
static void
board_reverse_opponents (board_t *board, const move_t move)
{
    bitboard_t adversary = (board_player (board) == BLACK_DISC) ?
                           board->white : board->black;
    bitboard_t player = (board_player (board) == WHITE_DISC) ?
                        board->white : board->black;

    for (size_t d = 0; d < DIRECTIONS; d++)
    {
        size_t gap = board_distance_between_no_opponents (board_size (board),
                                                          adversary, move, d);
        int i = move.row + row_direction[d] * gap;
        int j = move.column + column_direction[d] * gap;

        if (gap <= 1 || i < 0 || ((size_t) i) >= board->size ||
            j < 0 || ((size_t) j) >= board->size ||
            (player & set_bitboard (board->size, (size_t) i, (size_t) j)) == 0)
        {
            continue;
        }

        for (size_t t = 1; t < gap; t++)
        {
            board_set (board, board_player (board),
                       move.row + t * row_direction[d],
                       move.column + t * column_direction[d]);
        }
    }
}

bool
board_play (board_t *board, const move_t move)
{
    if (board != NULL && board->moves == 0)
    {
        switch (board->player)
        {
            case WHITE_DISC :
                board->moves = compute_moves (board->size, board->white,
                                              board->black);

                break;

            case BLACK_DISC :
                board->moves = compute_moves (board->size, board->black,
                                              board->white);

                break;

            default :
                return false;
        }
    }

    if (!board_is_move_valid (board, move)) /* If move is not possible. */
    {
        return false;
    }

    switch (board->player) /* Possible move. */
    {
        case WHITE_DISC :
            /* Place the player at the chosen position. */
            board->white |= set_bitboard (board->size, move.row, move.column);
            /* Reverse all opposents between 2 players discs. */
            board_reverse_opponents (board, move);
            /* Pass the hand to the opponent. */
            board_set_player (board, BLACK_DISC);
            /* Calculate news possible moves for opponent. */
            board->moves = compute_moves (board->size, board->black,
                                          board->white);

            break;

        case BLACK_DISC :
            board->black |= set_bitboard (board->size, move.row, move.column);
            board_reverse_opponents (board, move);
            board_set_player (board, WHITE_DISC);
            board->moves = compute_moves (board->size, board->white,
                                          board->black);

            break;

        default :
            return false;
    }

    /* If no possible move for the current player. */
    if (board_count_player_moves (board) == 0)
    {
        switch (board->player)
        {
            case WHITE_DISC :
                /* Set the hand to opponent. */
                board_set_player (board, BLACK_DISC);
                /* Calculate news possible moves for opponent. */
                board->moves = compute_moves (board->size, board->black,
                                              board->white);

                break;

            case BLACK_DISC :
                board_set_player (board, WHITE_DISC);
                board->moves = compute_moves (board->size, board->white,
                                              board->black);

                break;

            default :
                return false;
        }
    }

    /* If opponent can't move too => end of game. */
    if (board_count_player_moves (board) == 0)
    {
        board_set_player (board, EMPTY_DISC);
    }

    return true;
}

move_t
board_next_move (board_t *board)
{
    move_t next_move = (move_t)
                       {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};

    if (board == NULL || board->moves == 0)
    {
        return next_move;
    }

    if (board->next_move == 0)
    {
        board->next_move = board->moves;
    }

    for (size_t i = 0; i < board->size; i++)
    {
        for (size_t j = 0; j < board->size; j++)
        {
            bitboard_t bit = set_bitboard (board->size, i, j);

            if ((bit & board->next_move) != 0)
            {
                next_move.row = i;
                next_move.column = j;
                board->next_move &= ~bit;

                return next_move;
            }
        }
    }

    return next_move;
}


/************************ bitboard_t corner management ************************/

/* Get the bitboard contain bits at all corners positions. */
static bitboard_t
get_corners (const size_t size)
{
    if (!board_cheak_size (size))
    {
        return (bitboard_t) 0;
    }

    bitboard_t corners = (bitboard_t) 1;
    corners = (corners << (size - 1)) | corners;
    corners = (corners << ((size - 1) * size)) | corners;

    return corners;
}

bool
is_corner (const size_t size, const move_t move)
{
    /* Create a bitboard contain the bit of the move. */
    bitboard_t bitmove = set_bitboard (size, move.row, move.column);

    return ((bitmove & get_corners (size)) != (bitboard_t) 0);
}

move_t
get_corner_as_move (const size_t size, const short i)
{
    if (i < 0 || i > 3 || !board_cheak_size (size))
    {
        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    move_t result = (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};

    if ((i % 2) == 1)
    {
        result.column = size - 1;
    }
    else
    {
        result.column = 0;
    }

    if (i < 2)
    {
        result.row = 0;
    }
    else
    {
        result.row = size - 1;
    }

    return result;
}

/* Compute if the next_board have a joint movement with the actual board. */
static bitboard_t
joint_movement (const board_t *actual_board, const board_t *next_board)
{
    if (actual_board == NULL || next_board == NULL ||
        board_player (actual_board) == board_player (next_board))
    {
        return (bitboard_t) 0;
    }

    return actual_board->moves & next_board->moves;
}

/* Cast all the corner player moves as bitboard_t. */
static bitboard_t
get_playable_corners (const board_t *actual_board)
{
    if (actual_board == NULL)
    {
        return (bitboard_t) 0;
    }

    return actual_board->moves & get_corners (board_size (actual_board));
}

bitboard_t
get_corners_to_exam (const board_t *board)
{
    bitboard_t playable_corner = get_playable_corners (board);

    /* Verification if we have more than 1 corner available to play. */
    if (playable_corner == (bitboard_t) 0 ||
        bitboard_popcount (playable_corner) == 1)
    {
        return playable_corner;
    }

    size_t size = board_size (board);
    bitboard_t dangerous_corner = (bitboard_t) 0;
    bitboard_t corner0 = (bitboard_t) 1;
    bitboard_t corner1 = corner0 << (size - 1);
    bitboard_t corner3 = corner0 << (size * size - 1);
    bitboard_t corner2 = corner3 >> (size - 1);
    bitboard_t corners[4] = {corner0, corner1, corner2, corner3};

    /* On the 4-th corners. */
    for (short i = 0; i < 4; i++)
    {
        if ((corners[i] & playable_corner) == corners[i])
        {
            move_t current_corner_move = get_corner_as_move (size, i);
            board_t *copy = board_copy (board);
            board_play (copy, current_corner_move);
            bitboard_t bitjoint = joint_movement (board, copy);
            dangerous_corner |= (playable_corner & bitjoint);
            board_free (copy);
        }
    }

    if (dangerous_corner == (bitboard_t) 0)
    {
        return playable_corner;
    }
    else
    {
        return dangerous_corner;
    }
}


/*********************** bitboard_t borders management ************************/

bitboard_t*
get_borders (const size_t size)
{
    bitboard_t *result = malloc (4 * sizeof (bitboard_t));

    if (result == NULL)
    {
        return NULL;
    }

    if (!board_cheak_size (size))
    {
        bitboard_t zeros = (bitboard_t) 0;

        for (short i = 0; i < 4; i++)
        {
            result[i] = zeros;
        }

        return result;
    }

    bitboard_t one = (bitboard_t) 1;
    bitboard_t north = (one << size) - 1;
    bitboard_t south = north << ((size - 1) * size);
    bitboard_t west = one;

    for (size_t i = 0; i < size - 1; i++)
    {
        west = (west << size) | one;
    }

    bitboard_t east = west << (size - 1);
    result[0] = north;
    result[1] = south;
    result[2] = east;
    result[3] = west;

    return result;
}

bitboard_t*
get_boarders_init (size_t size)
{
    bitboard_t *result = malloc (4 * sizeof (bitboard_t));

    if (result == NULL)
    {
        return NULL;
    }

    if (!board_cheak_size (size))
    {
        bitboard_t zeros = (bitboard_t) 0;
        for (short i = 0; i < 4; i++)
        {
            result[i] = zeros;
        }

        return result;
    }

    bitboard_t north = (bitboard_t) 1;
    bitboard_t west = north;
    bitboard_t south = north << (size * (size - 1));
    bitboard_t east = north << (size - 1);
    result[0] = north;
    result[1] = south;
    result[2] = east;
    result[3] = west;

    return result;
}

size_t*
get_borders_increment (const size_t size)
{
    size_t *incr = malloc (4 * sizeof (size_t));

    if (!board_cheak_size (size) || incr == NULL)
    {
        return NULL;
    }

    incr[0] = 1;
    incr[1] = 1;
    incr[2] = size;
    incr[3] = size;

    return incr;
}

move_t
get_border_as_move (const bitboard_t bit, const size_t size, const short border)
{
    /* Verification if bit is in the indicated border. */
    bitboard_t *borders = get_borders (size);

    if (borders == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    bool test = (borders[border] & bit) == bit;
    free (borders);

    if (bitboard_popcount (bit) != 1 || !board_cheak_size (size) ||
        border < 0 || border > 3 || !test)
    {
        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    move_t result;
    bitboard_t bit_test;
    size_t increment;
    size_t cpt = 0;

    switch (border)
    {
        case 0 :
            result.row = 0;
            bit_test = (bitboard_t) 1;
            increment = 1;

            break;

        case 1 :
            result.row = size - 1;
            bit_test = ((bitboard_t) 1) << (size * (size - 1));
            increment = 1;

            break;

        case 2 :
            result.column = size - 1;
            bit_test = ((bitboard_t) 1) << (size - 1);
            increment = size;

            break;

        default :
            result.column = 0;
            bit_test = (bitboard_t) 1;
            increment = size;

            break;
    }

    while ((bit_test & bit) != bit)
    {
        cpt++;
        bit_test = bit_test << increment;
    }

    if (border < 2)
    {
        result.column = cpt;
    }
    else
    {
        result.row = cpt;
    }

    return result;
}

bitboard_t
get_interesting_borders (const board_t *board)
{
    bitboard_t zeros = (bitboard_t) 0;

    if (board == NULL || board_player (board) == EMPTY_DISC)
    {
        return zeros;
    }

    size_t size = board_size (board);
    bitboard_t *borders = get_borders (size);
    bitboard_t *borders_init = get_boarders_init (size);
    size_t *increment = get_borders_increment (size);

    if (borders == NULL || borders_init == NULL || increment == NULL)
    {
        if (borders != NULL)
        {
            free (borders);
        }

        if (borders_init != NULL)
        {
            free (borders_init);
        }

        if (increment != NULL)
        {
            free (increment);
        }

        return zeros;
    }

    disc_t player = board_player (board);
    bitboard_t interesting_borders = zeros;

    /* On all borders. */
    for (short i = 0; i < 4; i++)
    {
        bitboard_t playable_border = borders[i] & (board->moves);

        if (playable_border == zeros)
        {
            continue;
        }

        /* Check positions on the border selected. */
        for (size_t j = 1; j < size - 1; j++)
        {
            bitboard_t bit = borders_init[i] << (j * increment[i]);

            if ((bit & playable_border) == zeros)
            {
                continue;
            }

            move_t move = get_border_as_move (bit, size, i);
            board_t *copy = board_copy (board);

            if (copy == NULL)
            {
                continue;
            }

            board_play (copy, move);

            /* Test if end of game. */
            if (board_player (copy) == EMPTY_DISC)
            {
                int score  = bitboard_popcount (copy->black) -
                             bitboard_popcount (copy->white);
                score = (player == BLACK_DISC) ? score : -score;

                if (score >= 0)
                {
                    interesting_borders |= bit;
                }

                board_free (copy);

                continue;
            }

            bitboard_t new_opponent = (player == BLACK_DISC) ? copy->white :
                                                               copy->black;
            bitboard_t new_player = (player == BLACK_DISC) ? copy->black :
                                                             copy->white;
            bitboard_t opponent_move = (player == board_player (copy)) ?
                                       zeros : copy->moves;

            /* If opponent can't play, or hasn't anymore control on this border
             * it's an interesting move. */
            if (opponent_move == zeros ||
                ((new_opponent | opponent_move) & borders[i]) == zeros)
            {
                interesting_borders |= bit;
                board_free (copy);

                continue;
            }

            /* After play (with copy), check if it's a dangerous situation. */
            bitboard_t first_ally = bit;
            bitboard_t last_ally = bit;

            /* Looking for fist_allay. */
            for (size_t k = j - 1; k > 0; k--)
            {
                bitboard_t tmp = borders_init[i] << (k * increment[i]);

                if ((tmp & new_player) == zeros)
                {
                    break;
                }

                first_ally = tmp;
            }

            /* Looking for last_allay. */
            for (size_t k = j + 1; k < size; k++)
            {
                bitboard_t tmp = borders_init[i] << (k * increment[i]);

                if ((tmp & new_player) == zeros)
                {
                    break;
                }

                last_ally = tmp;
            }

            /* Test if first or last ally is a corner. */
            bool test = first_ally == borders_init[i];
            test |= last_ally == (borders_init[i] <<
                                 ((size - 1) * increment[i]));
            /* Test if the ally sequance is border by opponent disc. */
            test |= (((first_ally >> increment[i]) & new_opponent) != zeros) &&
                    (((last_ally << increment[i]) & new_opponent) != zeros);
            /* Test if the ally sequance is border by opponent possible move. */
            test |= (((first_ally >> increment[i]) & opponent_move) != zeros) &&
                    (((last_ally << increment[i]) & opponent_move) != zeros);

            if (test)
            {
                interesting_borders |= bit;
            }

            board_free (copy);
        }
    }

    free (borders);
    free (borders_init);
    free (increment);

    return interesting_borders;
}

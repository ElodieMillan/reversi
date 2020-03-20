#define _POSIX_C_SOURCE 200809L /* To use getline (). */

#include <player.h>

#include <ctype.h>
#include <string.h>


/********************************* Structures *********************************/

/* Alpha / Beta structure to store the state of both values. */
typedef struct
{
    int alpha;
    int beta;
} alpha_beta_t;


/*************************** Function declarations ****************************/

static int min (board_t *board, const size_t depth, const disc_t player_init);

static alpha_beta_t ab_min (board_t *board, const size_t depth,
                            const alpha_beta_t a_b, const disc_t player_init);

static alpha_beta_t newton_min (board_t *board, const size_t depth,
                                const alpha_beta_t a_b,
                                const disc_t player_init);

static alpha_beta_t ab_max (board_t *board, const size_t depth,
                            const alpha_beta_t a_b, const disc_t player_init);

static alpha_beta_t newton_max (board_t *board, const size_t depth,
                                const alpha_beta_t a_b,
                                const disc_t player_init);

static move_t ab_main_loop (const short ai, board_t *board, move_t best_move);


/********************************* Constants **********************************/

static bool rng_is_init = false;
static bool verbose = false;
static const int infinity = MAX_BOARD_SIZE * MAX_BOARD_SIZE * 3;
static size_t depth_ini = 0;

/* Function pointer of ab_min used. */
static alpha_beta_t (*ab_min_used[2]) (board_t *, const size_t,
                                       const alpha_beta_t, const disc_t) =
{ab_min, newton_min};

/* Function pointer of ab_max used. */
static alpha_beta_t (*ab_max_used[2]) (board_t *, const size_t,
                                       const alpha_beta_t, const disc_t) =
{ab_max, newton_max};


/***************************** Intern management ******************************/

void
set_verbose (void)
{
    verbose = true;
}

/* Initiate the rng with the time and the number of enter process.  */
static void
prng_init (void)
{
    if (!rng_is_init)
    {
        /* Take times and processor number as initial value. */
        srand (time (NULL) - getpid ());
        rng_is_init = true;
    }
}

/* In verbose mode, permit to print the move played by the player (strategy). */
static void
print_move_verbose (const move_t move, const disc_t player,
                    const short strategy)
{
    char str[2];
    str[0] = (char) move.row + '1';
    str[1] = '\0';
    char *start;

    switch (strategy)
    {
        case 0 :
            start = "Human";

            break;

        case 1 :
            start = "Random AI";

            break;

        case 2 :
            start = "Minimax AI";

            break;

        case 3 :
            start = "Alpha/Beta AI";

            break;

        case 4 :
            start = "Newton AI";

            break;

        default :
            return;
    }

    printf ("%s '%c' played the %c%s move.\n\n", start,
            player, (char) move.column + 'A', (move.row == 9) ? "10" : str);
}

/* Delete space on the string 'line'. */
static char*
delete_space (char *line)
{
    size_t i = 0; /* Rank of line. */
    size_t j = 0; /* Rank of line without space. */

    if (line == NULL)
    {
        return "";
    }

    line[strlen (line) - 1] = '\0';

    while (line[i] != '\0')
    {
        if (!isspace (line[i]))
        {
            line[j] = line[i];
            j++;
        }
        i++;
    }

    line[j] = '\0';

    return line;
}

/* Function that print a progress barre in the consol
 * when AI compute its move. */
static void
print_progress (const size_t count, const size_t max, const disc_t player_ini)
{
    char prefix[30] = "Wait the AI ' ' compute: 0% [";
    prefix[13] = player_ini;
	const char suffix[] = " 100%";
	const size_t prefix_length = sizeof(prefix) - 1;
	const size_t suffix_length = sizeof(suffix) - 1;
	char *buffer = calloc (max + prefix_length + suffix_length + 1, 1);
                                                               /* +1 for \0 */

    if (buffer == NULL)
    {
        return;
    }

    size_t i = 0;
	strcpy (buffer, prefix);

    for (i = 0; i < max; i++)
	{
		buffer[prefix_length + i] = i < count ? '#' : ' ';

        if (i == max - 1)
        {
            buffer[prefix_length + i] = ']';
        }
	}

    if (count == 0)
    {
        printf ("\n");
    }

    strcpy (&buffer[prefix_length + i], suffix);
    printf ("\033[A\33[2K%s\n", buffer);
	fflush (stdout);
	free (buffer);
}

static void
game_save (const board_t *board)
{
    if (board == NULL)
    {
        printf ("Error: Impossible to print a null board.");

        return;
    }

    /* Displaying message on the screen asking the user to input file name. */
    char *filename = "board.txt"; /* Default file name. */
    printf ("Give a filename to save the game (default: '%s'): ", filename);
    /* Initialize the file_name enter by operator. */
    char *file_name = NULL;
    size_t line_size = 0;
    bool use_filename = false; /* If false => use file_name give by operator. */
    getline (&file_name, &line_size, stdin);

    /* If file name is not write by operator = true. */
    if (file_name == NULL || file_name[0] == '\n')
    {
        use_filename = true; /* If true => use filename (default). */
    }

    /* Delete space on this file name. */
    file_name = delete_space (file_name);
    /* Displaying the number as output. */
    printf ("\nYou choose '%s' as filename.\n",
            (use_filename) ? filename : file_name);
    FILE *file = (use_filename) ?
                 fopen (filename, "w") : fopen (file_name, "w");

    if (file == NULL)
    {
        printf ("Error: The file %s can't be open.", file_name);

        return;
    }

    fprintf (file, "%c\n", board_player (board));

    for (size_t i = 0; i < board_size (board); i++)
    {
        for (size_t j = 0; j < board_size (board); j++)
        {
            char charac = board_get (board, i, j);
            fprintf (file, "%c ", (charac == HINT_DISC) ? EMPTY_DISC : charac);
        }

        fputc ('\n', file);
    }

    fclose (file);
    printf ("\nBoard saved in '%s'. \n", (use_filename) ? filename : file_name);
    free (file_name);
}

/* Return the difference of the score between the current player
 * and its opponent. */
static int
score_heuristic (const board_t *board, const disc_t player_init)
{
    int int_max = board_size (board) * board_size (board);
    score_t score = board_score (board);
    disc_t max_player = (score.black > score.white) ? BLACK_DISC : WHITE_DISC;
    int abs_score = (score.black > score.white) ? score.black - score.white :
                                                  score.white - score.black;
    int final_score;

    /* Game is over, returning 0 (draw), int_max (win) or -int_max (loose). */
    if (board_player (board) == EMPTY_DISC)
    {
        if (score.black == score.white)
        {
            final_score = 0;
        }
        else
        {
            final_score = (player_init == max_player) ? int_max + abs_score :
                                                        -(int_max + abs_score);
        }
    }
    else /* Game is not over. */
    {
        final_score = (player_init == max_player) ? abs_score : -abs_score;
    }

    return final_score;
}


/********************************* Heuristics *********************************/

/* ------------------------------ Human player ------------------------------ */

move_t
human_player (board_t *board)
{
    if (board == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE + 1,
                         .column = MAX_BOARD_SIZE + 1};
    }

    board_print (board, stdout);

    /* Incorrect move or wrong input => error message. */
    while (true)
    {
        printf ("Give your move (e.g. 'A5' or 'a5'), "
                "press 'q' or 'Q' to quit: ");
        /* Recovery of the movement write by operator. */
        char *move_chooses = NULL;
        size_t move_chooses_size = 0;
        getline (&move_chooses, &move_chooses_size, stdin);

        /* Error if nothing write. */
        if (move_chooses == NULL)
        {
            printf ("Wrong input, try again!\n\n");

            continue;
        }

        move_chooses = delete_space (move_chooses);
        char first_char = move_chooses[0];
        char second_char = move_chooses[1];
        char third_char = move_chooses[2];
        char fourth_char = move_chooses[3];

        if (first_char == '\n' || first_char == '\0')
        {
            printf ("Wrong input, try again!\n");
            free (move_chooses);

            continue;
        }

        if ((first_char == 'q' || first_char == 'Q') &&
            (second_char == '\n' || second_char == '\0'))
        {
            while (true)
            {
                printf ("Quitting, do you want to save this game (y/N)? ");
                char *quit_char = NULL;
                size_t quit_char_size = 0;
                getline (&quit_char, &quit_char_size, stdin);

                if (quit_char == NULL)
                {
                    printf ("Wrong input, try again!\n\n");
                    free (move_chooses);

                    continue;
                }

                quit_char = delete_space (quit_char);
                char yesno = quit_char[0];
                char other = quit_char[1];

                /* Verification if the first character is y or n. */
                if (yesno != 'Y' && yesno != 'y' &&
                    yesno != 'n' && yesno != 'N' &&
                    yesno != '\0' && yesno != '\n')
                {
                    printf ("Wrong input, try again!\n\n");
                    free (move_chooses);
                    free (quit_char);

                    continue;
                }

                /* If yesno = nothing we concidering the answer like No. */
                if (yesno == '\0' || yesno == '\n')
                {
                    free (move_chooses);
                    free (quit_char);

                    return (move_t) {.row = board_size (board),
                                     .column = board_size (board)};
                }

                /* Verification if the second caracter is "nothing". */
                if (other != '\0' && other != '\n')
                {
                    printf ("Wrong input, try again!\n\n");
                    free (quit_char);

                    continue;
                }

                /* If y or Y save game. */
                if (yesno == 'y' || yesno == 'Y')
                {
                    game_save (board);
                }

                free (move_chooses);
                free (quit_char);

                return (move_t) {.row = board_size (board),
                                 .column = board_size (board)};
            }
        }

        if (((first_char < 'A' || first_char >= 'A' + MAX_BOARD_SIZE) &&
             (first_char < 'a' || first_char >= 'a' + MAX_BOARD_SIZE)) ||
            (second_char < '1' || second_char >= '1' + MAX_BOARD_SIZE - 1) ||
            (third_char != '\n' && third_char != '\0' && third_char != '0'))
        {
            printf ("This move is invalid. Wrong input, try again!\n\n");
            free (move_chooses);

            continue;
        }

        /* Our row and column start at 0 index to MAX_BOARD_SIZE - 1
         * => we need to substract A (a) or 1 to start at 0. */
        size_t col = (first_char >= 'A' && first_char < 'A' + MAX_BOARD_SIZE) ?
                     first_char - 'A' : first_char - 'a';
        size_t row = second_char - '1';

        /* Case for line 10 (with 2 characters for the line and not 1). */
        if (third_char == '0' && second_char == '1')
        {
            if (board_size (board) < 10 ||
                (fourth_char != '\n' && fourth_char != '\0'))
            {
                printf ("This move is invalid. Wrong input, try again!\n");
                free (move_chooses);

                continue;
            }

            row = 9;
        }

        if (row >= board_size (board))
        {
            printf ("Row out of bounds. Wrong input, try again!\n");
            free (move_chooses);

            continue;
        }

        if (col >= board_size (board))
        {
            printf ("Column out of bounds. Wrong input, try again!\n");
            free (move_chooses);

            continue;
        }

        move_t move = (move_t) {.row = row, .column = col};

        if (!board_is_move_valid (board, move))
        {
            printf ("This move is invalid. Wrong input, try again! "
                    "(Choose a valid move from the '*').\n");
            free (move_chooses);

            continue;
        }

        free (move_chooses);

        if (verbose)
        {
            print_move_verbose (move, board_player (board), 0);
        }

        return move;
    }
}

/* --------------------------------- Random --------------------------------- */

move_t
random_player (board_t *board)
{
    if (board == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    /* Initiate the seed of the rng if it's not. */
    prng_init ();
    /* Compute a random number modulo the number of possible player moves. */
    size_t cpt_rand = rand () % board_count_player_moves (board);
    /* Get the 'cpt_rand' next move. */
    move_t player_move = board_next_move (board);

    while (cpt_rand != 0)
    {
        player_move = board_next_move (board);
        cpt_rand--;
    }

    if (verbose)
    {
        print_move_verbose (player_move, board_player (board), 1);
    }

    return player_move;
}

/* --------------------------------- Minimax -------------------------------- */

/* Return the max score of childrens next moves. */
static int
max (board_t *board, const size_t depth, const disc_t player_init)
{
    if (board == NULL)
    {
        /* Infinity because it will be minimise at the father node. */
        return infinity;
    }

    disc_t actual_player = board_player (board);

    /* Test of the position of the depth. */
    if (depth == (size_t) 0 || actual_player == EMPTY_DISC)
    {
        return score_heuristic (board, player_init);
    }

    /* Initiate the max_score and the intermediate value_score as -(size * size)
     * the lower possible score. */
    int max_value = -(board_size (board) * board_size (board));
    int value = -(board_size (board) * board_size (board));

    /* On all possible moves of this min node. */
    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            return -infinity;
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        if (board_player (copy) == EMPTY_DISC)
        {
            value = score_heuristic (copy, player_init);
        }
        else if (board_player (copy) == actual_player)
        {
            value = max (copy, (depth <= 2) ? 0 : depth - 2, player_init);
        }
        else
        {
            value = -min (copy, depth - 1, player_init);
        }

        /* Make the score_heuristic to weight the knot. */
        if (value > max_value)
        {
            max_value = value;
        }

        board_free (copy);
    }

    return max_value;
}

/* Return the min score of childrens next moves. */
static int
min (board_t *board, const size_t depth, const disc_t player_init)
{
    if (board == NULL)
    {
        /* - infinity because it will be maximise at the father node. */
        return -infinity;
    }

    disc_t opponent = board_player (board);

    /* Test of the position of the depth. */
    if (depth == (size_t) 0 || opponent == EMPTY_DISC)
    {
        return -score_heuristic (board, player_init);
    }

    /* Initiate the min_score and the intermediate value score as (size * size)
     * the biggest possible score. */
    int min_value = (board_size (board) * board_size (board));
    int value = (board_size (board) * board_size (board));

    /* For all possible moves we take the minimum of score_heuristic. */
    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            return infinity;
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        /* If game is over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            value = -score_heuristic (copy, player_init);
        }
        /* If opponent's turn. */
        else if (board_player (copy) == opponent)
        {
            value = min (copy, (depth <= 2) ? 0 : depth - 2, player_init);
        }
        /* Else player's turn. */
        else
        {
            value = -max (copy, depth - 1, player_init);
        }

        /* Make the score_heuristic to weight the knot. */
        if (value < min_value)
        {
            min_value = value;
        }

        board_free (copy);
    }

    return min_value;
}

move_t
minimax_player (board_t *board)
{
    if (board == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE + 1,
                         .column = MAX_BOARD_SIZE + 1};
    }

    switch (board_size (board))
    {
        case 4 :
            depth_ini = 8;

            break;

        case 6 :
            depth_ini = 5;

            break;

        case 8 :
            depth_ini = 4;

            break;

        default :
            depth_ini = 3;
    }

    disc_t player_init = board_player (board);

    if (verbose)
    {
        printf ("Wait the AI '%c' compute:\n", player_init);
    }

    /* Initiate the best score as - infinity. */
    int best_value = -infinity;
    int value = best_value;
    /* Declaration of the final return best move. */
    move_t best_move;
    size_t count = 0;
    size_t number_max_moves = board_count_player_moves (board);

    /* If just one possible move, do it. */
    if (number_max_moves == 1)
    {
        best_move = board_next_move (board);

        if (verbose)
        {
            printf ("\033[A\33[2K");
        }
    }
    /* Else, do the minimax algorithm to through all the possible movements
     * and make the best. */
    else
    {
        move_t possible_moves[number_max_moves];
        int cpt_move = 0;

        for (size_t i = 0; i < number_max_moves; i++)
        {
            /* Take a possible move. */
            move_t move = board_next_move (board);
            /* Copy the actual board. */
            board_t *copy = board_copy (board);

            if (copy == NULL)
            {
                board_free (copy);
                return (move_t) {.row = MAX_BOARD_SIZE + 1,
                                 .column = MAX_BOARD_SIZE + 1};
            }

            /* Play the next i-th possible moves. */
            board_play (copy, move);
            /* Minimisation of the score of opponent. */
            value = -min (copy, depth_ini, player_init);

            /* Compare with the actual best_value to maximise it. */
            if (value > best_value)
            {
                best_value = value;
                cpt_move = 1;
                possible_moves[0] = move;
            }
            else if (value == best_value)
            {
                possible_moves[cpt_move++] = move;
            }

            board_free (copy);

            /* To print the progress barre on the consol. */
            if (verbose)
            {
                count++;
                print_progress (count, number_max_moves, player_init);
            }
        }

        prng_init ();
        best_move = possible_moves[rand () % cpt_move];
    }

    if (verbose)
    {
        print_move_verbose (best_move, player_init, 2);
    }

    return best_move;
}

/* ------------------------------ Alpha / Beta ------------------------------ */

/* Calculate the alpha and beta for a max node. */
static alpha_beta_t
ab_max (board_t *board, const size_t depth, const alpha_beta_t a_b,
        const disc_t player_init)
{
    if (board == NULL)
    {
        return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        /* Only in C11 version. */
    }

    /* The player is opponent in ab_min. */
    disc_t player = board_player (board);
    /* Tampon alpha beta initiate at alpha and beta value to enter. */
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};

    /* Test if depth == 0 or game is over. */
    if (depth == (size_t) 0)
    {
        tampon_ab.alpha = score_heuristic (board, player_init);

        if (tampon_ab.alpha > result_ab.alpha)
        {
            result_ab.alpha = tampon_ab.alpha;
        }

        return result_ab;
    }

    /* For all possible moves we take the minimum of score_heuristic. */
    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            board_free (copy);
            return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        /* If game is over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            /* Alpha take the maximum value between score and alpha. */
            tampon_ab.beta = score_heuristic (copy, player_init);

            if (tampon_ab.beta > result_ab.alpha)
            {
                result_ab.alpha = tampon_ab.beta;
            }
        }
        else if (board_player (copy) == player)
        {
            tampon_ab = ab_max (copy, (depth <= 2) ? 0 : depth - 2,
                                result_ab, player_init);

            /* Test if this beta is better and copy the beta of the min son
             * in this min. */
            if (tampon_ab.alpha > result_ab.alpha)
            {
                result_ab.alpha = tampon_ab.alpha;
            }
        }
        else
        {
            tampon_ab = ab_min (copy, depth - 1, result_ab, player_init);

            /* Alpha in the max become beta in the min. */
            if (tampon_ab.beta > result_ab.alpha)
            {
                result_ab.alpha = tampon_ab.beta;
            }
        }

        board_free (copy);

        /* In max node, if a >= b, we can quit this max node. */
        if (result_ab.alpha >= result_ab.beta)
        {
            break;
        }
    }

    return result_ab;
}

/* Calculate the alpha and beta for a min node. */
static alpha_beta_t
ab_min (board_t *board, const size_t depth, const alpha_beta_t a_b,
        const disc_t player_init)
{
    if (board == NULL)
    {
        return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
    }

    /* The player is opponent in ab_min. */
    disc_t opponent = board_player (board);
    /* result alpha beta initiate at alpha and beta value to enter. */
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};

    /* Test if depth == 0 or game is over. */
    if (depth == (size_t) 0)
    {
        tampon_ab.beta = score_heuristic (board, player_init);

        if (tampon_ab.beta < result_ab.beta)
        {
            result_ab.beta = tampon_ab.beta;
        }

        return result_ab;
    }

    /* For all possible moves we take the minimum of score_heuristic. */
    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            board_free (copy);
            return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        /* If game is over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            /* Alpha take the maximum value between score and alpha. */
            tampon_ab.alpha = score_heuristic (copy, player_init);

            if (tampon_ab.alpha < result_ab.alpha)
            {
                result_ab.beta = tampon_ab.alpha;
            }
        }
        else if (board_player (copy) == opponent)
        {
            /* Min node. */
            tampon_ab = ab_min (copy, (depth <= 2) ? 0 : depth - 2,
                                result_ab, player_init);

            if (tampon_ab.beta < result_ab.beta)
            {
                result_ab.beta = tampon_ab.beta;
            }
        }
        else
        {
            /* Max node. */
            tampon_ab = ab_max (copy, depth - 1, result_ab, player_init);

            /* Alpha in the max become beta in the min. */
            if (tampon_ab.alpha < result_ab.beta)
            {
                result_ab.beta = tampon_ab.alpha;
            }
        }

        board_free (copy);

        /* If alpha >= beta stop, else pass at the next iteration. */
        if (result_ab.alpha >= result_ab.beta)
        {
            break;
        }
    }

    return result_ab;
}

move_t
minimax_ab_player (board_t *board)
{
    if (board == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE + 1,
                         .column = MAX_BOARD_SIZE + 1};
    }

    switch (board_size (board))
    {
        case 4 :
            depth_ini = 12;

            break;

        case 6 :
            depth_ini = 10;

            break;

        case 8 :
            depth_ini = 7;

            break;

        default :
            depth_ini = 5;
    }

    disc_t player_init = board_player (board);

    if (verbose)
    {
        printf ("Wait the AI '%c' compute:\n", player_init);
    }

    /* Keep the verbose test. */
    bool is_it_verbose_true = verbose;
    /* Make verbose false to don't do printf in the random_player (). */
    verbose = false;
    /* By default, best move is random -> for the badest possibility that AI
    * don't have a best_move, it need to choose one finally. */
    move_t best_move = random_player (board);

    /* Make verbose true again if it was true before. */
    if (is_it_verbose_true)
    {
        verbose = true;
    }

    size_t number_max_moves = board_count_player_moves (board);

    if (number_max_moves == 1)
    {
        best_move = board_next_move (board);

        if (verbose)
        {
            printf ("\033[A\33[2K"); /* Don't write the last printf. */
        }
    }
    else
    {
        /* Ai pointer function = 0. */
        best_move = ab_main_loop (0, board, best_move);
    }

    if (verbose)
    {
        print_move_verbose (best_move, player_init, 3);
    }

    return best_move;
}

/* ----------------------------- Newton tactics ----------------------------- */

/* Calculate the alpha and beta for a max node in newton_player. */
static alpha_beta_t
newton_max (board_t *board, const size_t depth, const alpha_beta_t a_b,
            const disc_t player_init)
{
    if (board == NULL)
    {
        return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        /* Only in C11 version */
    }

    /* The player is opponent in min. */
    disc_t player = board_player (board);
    /* Tampon alpha beta initiate at alpha and beta value to enter. */
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};

    /* Test if depth == 0 or game is over. */
    if (depth == (size_t) 0)
    {
        tampon_ab.alpha = score_heuristic (board, player_init);

        if (tampon_ab.alpha > result_ab.alpha)
        {
            result_ab.alpha = tampon_ab.alpha;
        }

        return result_ab;
    }

    /* For all possible moves we take the minimum of score_heuristic. */
    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);
        size_t size = board_size (board);
        const int int_max = size * size;

        /* If at this max node, AI can play corner,
         * then valorize this branch. */
        if (is_corner (size, move) && depth == depth_ini - 2)
        {
            return (alpha_beta_t) {.alpha = int_max, .beta = a_b.beta};
        }

        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        /* If game is over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            /* Alpha take the maximum value between score and alpha. */
            tampon_ab.beta = score_heuristic (copy, player_init);

            if (tampon_ab.beta > result_ab.alpha)
            {
                result_ab.alpha = tampon_ab.beta;
            }
        }
        else if (board_player (copy) == player)
        {
            /* Max node. */
            tampon_ab = newton_max (copy, (depth <= 2) ? 0 : depth - 2,
                                    result_ab, player_init);

            if (tampon_ab.alpha > result_ab.alpha)
            {
                result_ab.alpha = tampon_ab.alpha;
            }
        }
        else
        {
            /* Min node. */
            tampon_ab = newton_min (copy, depth - 1, result_ab,
                                    player_init);

            if (tampon_ab.beta > result_ab.alpha)
            {
                /* Beta in min become alpha in max. */
                result_ab.alpha = tampon_ab.beta;
            }
        }

        board_free (copy);

        /* In max node, if a >= b, we can quit this max node. */
        if (result_ab.alpha >= result_ab.beta)
        {
            break;
        }
    }

    return result_ab;
}

/* Calculate the alpha and beta for a min node in newton_player. */
static alpha_beta_t
newton_min (board_t *board, const size_t depth, const alpha_beta_t a_b,
            const disc_t player_init)
{
    if (board == NULL)
    {
        return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
    }

    /* The player is opponent in min. */
    disc_t opponent = board_player (board);
    /* Result alpha beta initiate at alpha and beta value to enter. */
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = a_b.alpha, .beta = a_b.beta};

    /* Test if depth == 0 or game is over. */
    if (depth == (size_t) 0)
    {
        tampon_ab.beta = score_heuristic (board, player_init);

        if (tampon_ab.beta < result_ab.beta)
        {
            result_ab.beta = tampon_ab.beta;
        }

        return result_ab;
    }

    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        /* Take a possible move. */
        move_t move = board_next_move (board);

        /* If opposent's move is a corner,
         * then stop this branch. */
        if (is_corner (board_size (board), move) && depth == depth_ini - 1)
        {
            /* Return just alpha > beta at infinity to be sure that this branch
             * will not be taken. */
            return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        }

        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            return (alpha_beta_t) {.alpha = infinity, .beta = -infinity};
        }

        /* Play the next i-th possible moves on this copy. */
        board_play (copy, move);

        /* If game is over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            /* Alpha take the maximum value between score and alpha. */
            tampon_ab.alpha = score_heuristic (copy, player_init);

            if (tampon_ab.alpha < result_ab.alpha)
            {
                result_ab.beta = tampon_ab.alpha;
            }
        }
        else if (board_player (copy) == opponent)
        {
            /* Min node. */
            tampon_ab = newton_min (copy, (depth <= 2) ? 0 : depth - 2,
                                    result_ab, player_init);

            if (tampon_ab.beta < result_ab.beta)
            {
                result_ab.beta = tampon_ab.beta;
            }
        }
        else
        {
            /* Max node. */
            tampon_ab = newton_max (copy, depth - 1, result_ab,
                                    player_init);

            if (tampon_ab.alpha < result_ab.beta)
            {
                /* Alpha in the max become beta in the min. */
                result_ab.beta = tampon_ab.alpha;
            }
        }

        board_free (copy);

        /* If alpha >= beta stop, else pass at the next iteration. */
        if (result_ab.alpha >= result_ab.beta)
        {
            break;
        }
    }

    return result_ab;
}

/* Make the loop on all possible and interesting corners that AI can do. */
static move_t
newton_corner_loop (const short ai, board_t *board, move_t best_move,
                    bitboard_t corners[4], bitboard_t corner)
{
    size_t count = 0;
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    disc_t player_init = board_player (board);
    size_t number_max_moves = board_count_player_moves (board);

    /* We need to take the corner that need to be save first! */
    for (short i = 0; i < 4; i++)
    {
        if ((corner & corners[i]) == (bitboard_t) 0)
        {
            continue;
        }

        move_t move = get_corner_as_move (board_size (board), i);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            board_free (copy);

            return (move_t) {.row = MAX_BOARD_SIZE + 1,
                             .column = MAX_BOARD_SIZE + 1};
        }

        /* Play the next i-th possible moves. */
        board_play (copy, move);

        if (board_player (copy) == EMPTY_DISC)
        {
            int score = score_heuristic (copy, player_init);

            if (verbose && score != 0)
            {
                count++;
                print_progress (count, number_max_moves, player_init);
            }

            /* If player init win, just do it. */
            if (score > 0)
            {
                board_free (copy);
                best_move = move;

                break;
            }
            /* If player init lost, don't select this move. */
            else if (score < 0)
            {
                board_free (copy);

                continue;
            }
            /* Else score = 0, let evaluate it. */
            else
            {
                tampon_ab.beta = score;
            }
        }
        /* Test if it's opponent -> minimisation. */
        else if (board_player (copy) != player_init)
        {
            tampon_ab  = ab_min_used[ai] (copy, depth_ini, result_ab,
                                          player_init);
        }
        /* Test if it's same player -> maximisation. */
        else
        {
            tampon_ab  = ab_max_used[ai] (copy, depth_ini, result_ab,
                                          player_init);
        }

        /* If alpha >= beta, pass at the next move. */
        if (tampon_ab.alpha >= tampon_ab.beta)
        {
            if (verbose)
            {
                count++;
                print_progress (count, number_max_moves, player_init);
            }

            board_free (copy);

            continue;
        }

        /* If beta son > actual alpha, it's a better move.*/
        if (tampon_ab.beta > result_ab.alpha)
        {
            /* In max node, beta son become alpha. */
            result_ab.alpha = tampon_ab.beta;
            best_move = move;
        }

        board_free (copy);

        /* To print the progress barre on the consol. */
        if (verbose)
        {
            count++;
            print_progress (count, number_max_moves, player_init);
        }
    }

    /* To print the final barre on the consol. */
    if (verbose)
    {
        print_progress (number_max_moves, number_max_moves, player_init);
    }

    return best_move;
}

/* Make the loop on all possible and interesting borders that AI can do. */
static move_t
newton_border_loop (const short ai, bitboard_t playable_borders,
                        board_t *board)
{
    size_t size = board_size (board);
    size_t* increment = get_borders_increment (size);
    bitboard_t* borders = get_borders (size);
    bitboard_t* borders_init = get_boarders_init (size);

    if (board == NULL || increment == NULL ||
        borders == NULL || borders_init == NULL)
    {
        if (increment != NULL)
        {
            free (increment);
        }

        if (borders != NULL)
        {
            free (borders);
        }

        if (borders_init != NULL)
        {
            free (borders_init);
        }

        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    size_t count = 0;
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    disc_t player_init = board_player (board);
    size_t number_max_moves = board_count_player_moves (board);
    move_t best_move;
    bool out = false;

    for (short i = 0; i < 4; i++)
    {
        if (out)
        {
            break;
        }

        /* Check how borders is this border. */
        if ((borders[i] & playable_borders) == (bitboard_t) 0)
        {
            continue;
        }

        /* Check the position on the i-th border (without corner). */
        for (size_t j = 1; j < size - 1; j++)
        {
            bitboard_t bit = borders_init[i] << (j * increment[i]);

            if ((bit & playable_borders) != bit)
            {
                continue;
            }

            move_t move = get_border_as_move (bit, size, i);
            board_t *copy = board_copy (board);

            if (copy == NULL)
            {
                return (move_t) {.row = MAX_BOARD_SIZE + 1,
                                 .column = MAX_BOARD_SIZE + 1};
            }

            /* Play the next i-th possible moves. */
            board_play (copy, move);

            if (board_player (copy) == EMPTY_DISC)
            {
                int score = score_heuristic (copy, player_init);

                if (verbose && score != 0)
                {
                    count++;
                    print_progress (count, number_max_moves, player_init);
                }

                /* If player init win, just do it. */
                if (score > 0)
                {
                    board_free (copy);
                    best_move = move;
                    out = true;

                    break;
                }
                /* If player init lost, don't select this move. */
                else if (score < 0)
                {
                    board_free (copy);

                    continue;
                }
                /* Else score = 0, let evaluate it. */
                else
                {
                    tampon_ab.beta = score;
                }
            }
            /* Test if it's opponent -> minimisation. */
            else if (board_player (copy) != player_init)
            {
                tampon_ab  = ab_min_used[ai] (copy, depth_ini, result_ab,
                                              player_init);
            }
            /* Test if it's same player -> maximisation. */
            else
            {
                tampon_ab  = ab_max_used[ai] (copy, depth_ini, result_ab,
                                              player_init);
            }

            /* If alpha >= beta, pass at the next move. */
            if (tampon_ab.alpha >= tampon_ab.beta)
            {
                if (verbose)
                {
                    count++;
                    print_progress (count, number_max_moves, player_init);
                }

                board_free (copy);

                continue;
            }

            /* If beta son > actual alpha, it's a better move. */
            if (tampon_ab.beta > result_ab.alpha)
            {
                /* In max node, beta son become alpha. */
                result_ab.alpha = tampon_ab.beta;
                best_move = move;
            }

            board_free (copy);

            /* To print the progress barre on the consol. */
            if (verbose)
            {
                count++;
                print_progress (count, number_max_moves, player_init);
            }
        }
    }

    if (verbose)
    {
        print_progress (number_max_moves, number_max_moves, player_init);
    }

    free (increment);
    free (borders);
    free (borders_init);

    return best_move;
}

move_t
newton_player (board_t *board)
{
    if (board == NULL)
    {
        return (move_t) {.row = MAX_BOARD_SIZE + 1,
                         .column = MAX_BOARD_SIZE + 1};
    }

    switch (board_size (board))
    {
        case 4 :
            depth_ini = 12;

            break;

        case 6 :
            depth_ini = 10;

            break;

        case 8 :
            depth_ini = 7;

            break;

        default :
            depth_ini = 5;
    }

    disc_t player_init = board_player (board);

    if (verbose)
    {
        printf ("Wait the AI '%c' compute:\n", player_init);
    }

    /* Keep the verbose test. */
    bool is_verbose = verbose;
    /* Make verbose false to don't do printf in the random_player (). */
    verbose = false;
    /* By default, best move is random -> for the badest possibility that AI
     * don't have a best_move, it need to choose one finally. */
    move_t best_move = random_player (board);
    /* Make verbose true again if it was true before. */
    verbose = is_verbose;
    size_t number_max_moves = board_count_player_moves (board);

    if (number_max_moves == 1)
    {
        if (verbose)
        {
            printf ("\033[A\33[2K"); /* Don't write the last printf. */
        }

        return board_next_move (board);
    }

    size_t size = board_size (board);

    /* Management of the corners. */
    bitboard_t corner = get_corners_to_exam (board);
    size_t count = bitboard_popcount (corner);
    bitboard_t corner0 = (bitboard_t) 1;
    bitboard_t corner1 = corner0 << (size - 1);
    bitboard_t corner3 = corner0 << (size * size - 1);
    bitboard_t corner2 = corner3 >> (size - 1);
    bitboard_t corners[4] = {corner0, corner1, corner2, corner3};
    short ai = 1; /* Pointer function is 1 for newton_player. */

    /* If just one corner -> through it and do it. */
    if (count == 1)
    {
        for (short i = 0; i < 4; i++)
        {
            if ((corner & corners[i]) == corners[i])
            {
                best_move = get_corner_as_move (size, i);

                if (verbose)
                {
                    printf ("\033[A\33[2K");
                    print_move_verbose (best_move, player_init, 4);
                }

                return best_move;
            }
        }
    }
    /* If more than 1 corner -> make the loop alpha/beta to through the best. */
    else if (count > 1)
    {
        best_move = newton_corner_loop (ai, board, best_move, corners, corner);

        if (verbose)
        {
            print_move_verbose (best_move, player_init, 4);
        }

        return best_move;
    }

    /* Management of the borders. */
    bitboard_t interesting_borders = get_interesting_borders (board);
    count = bitboard_popcount (interesting_borders);

    /* If just one border -> through it and do it. */
    if (count == 1)
    {
        bitboard_t *borders = get_borders (size);

        for (short i = 0; i < 4; i++)
        {
            if ((borders[i] & interesting_borders) == interesting_borders)
            {
                best_move = get_border_as_move (interesting_borders, size, i);

                if (verbose)
                {
                    printf ("\033[A\33[2K");
                    print_move_verbose (best_move, player_init, 4);
                }
                free (borders);

                return best_move;
            }
        }
    }
    /* If more than 1 border -> make the loop alpha/beta to through the best. */
    else if (count > 1)
    {
        best_move = newton_border_loop (ai, interesting_borders, board);

        if (verbose)
        {
            print_move_verbose (best_move, player_init, 4);
        }

        return best_move;
    }

    best_move = ab_main_loop (ai, board, best_move); /* ai = 1. */

    /* If no corners and no border, get the normal alpha/beta compute. */
    if (verbose)
    {
        print_move_verbose (best_move, player_init, 4);
    }

    return best_move;
}

/* --------------------- Alpha / Beta & Newton main loop -------------------- */

/* Execute main loop of ab_player and newton_player functions. */
static move_t
ab_main_loop (const short ai, board_t *board, move_t best_move)
{
    if (verbose)
    {
        printf ("\033[A\33[2K"); /* Don't write the last printf. */
    }

    if (board == NULL || ai < 0 || ai > 1)
    {
        return (move_t) {.row = MAX_BOARD_SIZE, .column = MAX_BOARD_SIZE};
    }

    /* Initialize a at -infinity and b at +infinity. */
    alpha_beta_t result_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    alpha_beta_t tampon_ab = (alpha_beta_t)
                             {.alpha = -infinity, .beta = infinity};
    disc_t player_init = board_player (board);
    size_t number_max_moves = board_count_player_moves (board);

    for (size_t i = 0; i < board_count_player_moves (board); i++)
    {
        if (verbose)
        {
            print_progress (i, number_max_moves, player_init);
        }

        /* Take a possible move. */
        move_t move = board_next_move (board);
        /* Copy the actual board. */
        board_t *copy = board_copy (board);

        if (copy == NULL)
        {
            return (move_t) {.row = MAX_BOARD_SIZE + 1,
                             .column = MAX_BOARD_SIZE + 1};
        }

        /* Play the next i-th possible moves. */
        board_play (copy, move);

        /* Test if next is game over. */
        if (board_player (copy) == EMPTY_DISC)
        {
            int score = score_heuristic (copy, player_init);

            /* If player init win, just do it. */
            if (score > 0)
            {
                board_free (copy);
                best_move = move;

                break;
            }
            /* If player init lost, don't select this move. */
            else if (score < 0)
            {
                board_free (copy);

                continue;
            }
            /* Else score = 0, let evaluate it. */
            else
            {
                tampon_ab.beta = score;
            }
        }
        /* Test if it's opponent -> minimisation. */
        else if (board_player (copy) != player_init)
        {
            tampon_ab  = ab_min_used[ai] (copy, depth_ini, result_ab,
                                          player_init);
        }
        /* Test if it's same player -> maximisation. */
        else
        {
            tampon_ab  = ab_max_used[ai] (copy, depth_ini, result_ab,
                                          player_init);
        }

        /* If alpha >= beta, pass at the next move. */
        if (tampon_ab.alpha >= tampon_ab.beta)
        {
            board_free (copy);

            continue;
        }

        /* If beta son > actual alpha, it's a better move. */
        if (tampon_ab.beta > result_ab.alpha)
        {
            /* In max node, beta son become alpha. */
            result_ab.alpha = tampon_ab.beta;
            best_move = move;
        }

        board_free (copy);
    }

    if (verbose)
    {
        print_progress (number_max_moves, number_max_moves, player_init);
    }

    return best_move;
}

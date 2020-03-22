#include "reversi.h"

#include <ctype.h>
#include <err.h>
#include <getopt.h>

#include <player.h>


/********************************* Constants **********************************/

static bool verbose = false;
static bool contest_mode = false;
static bool all = false;
static int contest_ai = 4;
static int black_ai = 0;
static int white_ai = 0;

/* String description for all possible players */
static char (*char_player_used[5]) =
{"human", "random AI", "minimax AI", "alpha/beta AI", "Newton AI"};

/* Function's pointers for all possible players */
static move_t (*player_used[5]) (board_t *) =
{human_player, random_player, minimax_player, minimax_ab_player, newton_player};


/****************************** Intern management *****************************/

/* Print help instructions. */
static void
help ()
{
    printf ("\n**************** Welcome to the reversi Game *****************\n"
            "\nUsage: reversi [-s SIZE|-b[N]|-w[N]|-c[N]|-v|-V|-h] [FILE]"
            "\nPlay a reversi game with human or program players.\n"
            "  -s, --size SIZE\tboard size (min=1, max=5 (default: 4))\n"
            "  -b, --black-ai [N]\tset tactic of black player (default: 0)\n"
            "  -w, --white-ai [N]\tset tactic of white player (default: 0)\n"
            "  -c, --contest [N]\tenable 'contest' mode and set it's tactic\n"
            "\t\t\t(default: 4)\n"
            "  -a, --all \t\tpermit to parse all files\n"
            "  -v, --verbose\t\tverbose output\n"
            "  -V, --version\t\tdisplay version and exit\n"
            "  -h, --help\t\tdisplay this help and exit\n"
            "\nTactic list:    \tSize list:\n"
            "  0 : human       \t  1 : 2x2\n"
            "  1 : random      \t  2 : 4x4\n"
            "  2 : minimax     \t  3 : 6x6\n"
            "  3 : alpha/beta  \t  4 : 8x8\n"
            "  4 : Newton      \t  5 : 10x10\n\n"
            "Example : ./reversi -s3 -b4 -w1 -v \n"
            "          for a 6x6 size, white human and black AI Newton with\n"
            "          verbose mode.\n\n"
            "************************* ENJOY =) *************************\n\n");
}

/* Print the version of this program. */
static void
version ()
{
    printf ("\nreversi %d.%d.%d\n"
            "This software allows to play to reversi game.\n\n",
            VERSION, SUBVERSION, REVISION);
}


/*********************************** Parsing **********************************/

/* Parse the file given that contains the board to play. */
static board_t*
file_parser (const char *filename)
{
    FILE *file = fopen (filename, "r");

    if (file == NULL)
    {
        warnx ("Error: The file %s can't be open.", filename); /* No err() */
         /* because I parse an undifined number of files with option -a    */
         /* (cf : main function).                                          */

        return NULL;
    }

    int character = fgetc (file);
    disc_t player = EMPTY_DISC; /* Initialisation. */
    char first_row[MAX_BOARD_SIZE];
    short index = 0;
    short number_rows = 0;
    short number_columns = 0;
    board_t *game_board = NULL;

    while (character != EOF)
    {
        switch (character)
        {
            /* Ignore' ' and '\t'. */
            case ' ' : case '\t' :
                character = fgetc (file);

                break;

            /* Ignore comments start by '#'. */
            case '#' :
                do
                {
                    character = fgetc (file);
                }
                while (character != '\n' && character != EOF);

                break;

            case '\n' :
                if (index == 0)
                {
                    /* Empty ligne => do nothing. */
                }
                else if (number_rows == 0) /* End first row. */
                {
                    if (!board_cheak_size (index))
                    {
                        warnx ("Error: The size is not correct.");
                        fclose (file);

                        return NULL;
                    }

                    number_columns = index;
                    game_board = board_alloc (index, player);

                    if (game_board == NULL)
                    {
                      warnx ("Error: file %s: impossible to allocate memory.",
                             filename);
                      fclose (file);

                      return NULL;
                    }

                    for (int i = 0; i < index; i++)
                    {
                        board_set (game_board, first_row[i], 0, i);
                    }

                    number_rows++;
                }
                /* Verification number of columns. */
                else
                {
                    if (number_columns > index)
                    {
                        warnx ("Error: The line %d is not complet.\n"
                                "We need %d discs more.",
                                number_rows + 1, number_columns - index);
                        fclose (file);
                        board_free (game_board);

                        return NULL;
                    }

                    number_rows++;
                }

                index = 0;
                character = fgetc (file);

                break;

            case EMPTY_DISC :
                if (player == EMPTY_DISC)
                {
                    warnx ("Error: The player '%c' is not correct.", player);
                    fclose (file);
                    board_free (game_board);

                    return NULL;
                }
                else if (number_rows == 0)
                {
                    if (index >= 10)
                    {
                        warnx ("Error: The first row is too big and contains "
                               "more than 10 character.");
                        fclose (file);
                        board_free (game_board);

                        return NULL;
                    }

                    first_row[index++] = character;
                }
                /* The board is already full. */
                else if (number_rows == number_columns)
                {
                    warnx ("Error: The board is not a square: it contains %d "
                           "column(s) and more than %d rows(s).",
                           number_columns, number_columns);
                    fclose (file);
                    board_free (game_board);

                    return NULL;
                }
                else if (number_columns == index)
                {
                    warnx ("Error: Too mutch character at line %d.",
                           number_rows + 1);
                    fclose (file);
                    board_free (game_board);

                    return NULL;
                }
                else
                {
                    board_set (game_board, character, number_rows, index++);
                }

                character = fgetc (file);

                break;

            case BLACK_DISC : case WHITE_DISC :
                if (player == EMPTY_DISC)
                {
                    player = character;
                }
                else if (number_rows == 0)
                {
                    if (index >= 10)
                    {
                        warnx ("Error: The first row is too big and contains "
                               "more than 10 character.");
                        fclose (file);
                        board_free (game_board);

                        return NULL;
                    }

                    first_row[index++] = character;
                }
                /* The board is already full. */
                else if (number_rows == number_columns)
                {
                    warnx ("Error: The board is not a square: it contains %d "
                           "column(s) and more than %d rows(s).",
                           number_columns, number_columns);
                    fclose (file);
                    board_free (game_board);

                    return NULL;
                }
                else if (number_columns == index)
                {
                    warnx ("Error: Too mutch character at line %d.",
                           number_rows + 1);
                    fclose (file);
                    board_free (game_board);

                    return NULL;
                }
                else
                {
                    board_set (game_board, character, number_rows, index++);
                }

                character = fgetc (file);

                break;

            default :
                warnx ("Error: The file %s contain wrong character '%c' at line"
                       " %d.", filename, character, number_rows + 1);
                fclose (file);
                board_free (game_board);

                return NULL;
        }
    }

    /* If EOF. */
    if (index != 0)
    {
        if (index < number_columns)
        {
            warnx ("Error: The ligne %d is unfinished : missing %d character.",
                   number_rows + 1, number_columns - index);
            fclose (file);
            board_free (game_board);

            return NULL;
        }

        number_rows++;
    }

    if (number_rows == 0)
    {
        warnx ("Error: The file %s is empty.", filename);
        fclose (file);
        board_free (game_board);

        return NULL;
    }

    if (number_columns > number_rows)
    {
        warnx ("Error: This board in not a square.\n"
               "Its size is %d column(s) x %d row(s).",
               number_rows, number_columns);
        fclose (file);
        board_free (game_board);

        return NULL;
    }

    fclose (file);

    if (board_player (game_board) == EMPTY_DISC)
    {
        return game_board;
    }

    /* If no possible move. */
    if (board_count_player_moves (game_board) == 0)
    {
        board_set_player (game_board,
                          (board_player (game_board) == BLACK_DISC) ?
                          WHITE_DISC : BLACK_DISC);
    }

    if (board_count_player_moves (game_board) == 0)
    {
        board_set_player (game_board, EMPTY_DISC);
    }

    return game_board;
}


/************************************ Game ************************************/

/* Run the main loop which handle the alternation of players and play a whole
 * game. */
static int
game (move_t (*black) (board_t *), move_t (*white) (board_t *), board_t *board)
{
    /* Verification if we don't have player => end game. */
    if (board_player (board) == EMPTY_DISC)
    {
        score_t score = board_score (board);

        if (score.black > score.white)
        {
            printf ("Player %c win the game.\n", BLACK_DISC);

            return 1;
        }
        else if (score.black < score.white)
        {
            printf ("Player %c win the game.\n", WHITE_DISC);

            return 2;
        }
        else
        {
            printf ("Draw game, no winner.\n");

            return 0;
        }
    }

    /* First message at start. */
    printf ("\nWelcome to this reversi game!\n"
            "Black player (%c) is %s and white player (%c) is %s.\n"
            "%s player start!\n\n", BLACK_DISC, char_player_used[black_ai],
            WHITE_DISC, char_player_used[white_ai],
            (board_player (board) == BLACK_DISC) ? "Black" : "White");

    /* Alternate between the players (a player can pass his turn). */
    while (board_player (board) != EMPTY_DISC)
    {
        /* Recovers the move do by the current player. */
        move_t move = (board_player (board) == BLACK_DISC) ?
                      black (board) : white (board);

        /* Test if player want to interupt the game. */
        if (move.row == board_size (board) && move.column == board_size (board))
        {
            printf ("\nPlayer '%c' resigned. Player '%c' win the game.\n",
                    board_player (board), (board_player (board) == BLACK_DISC) ?
                    WHITE_DISC : BLACK_DISC);

            return (board_player (board) == BLACK_DISC) ? -1 : -2;
        }

        board_play (board, move);
    }

    /* End of game: nobody concedes. */
    score_t score = board_score (board);

    if (score.black != score.white)
    {
        printf ("\nPlayer '%c' win the game.\n",
                (score.black > score.white) ? BLACK_DISC : WHITE_DISC);

        return (score.black > score.white) ? 1 : 2;
    }
    else
    {
        printf ("\nDraw game, no winner.\n");

        return 0;
    }
}


/************************************ Main ************************************/

int
main (int argc, char* argv[])
{
    int optc;
    size_t board_size = 8;
    char *op = "b::w::s:c::avVh";

    struct option long_opts[] =
    {
        {"black-ai", optional_argument, NULL, 'b'},
        {"white-ai", optional_argument, NULL, 'w'},
        {"size", required_argument, NULL, 's'},
        {"contest", optional_argument, NULL, 'c'},
        {"all", no_argument, NULL, 'a'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'},
        {NULL,0,NULL,0}
    };

    while ((optc = getopt_long (argc, argv, op, long_opts, NULL)) != -1)
    {
        switch (optc)
        {
            case 'b' :
                if (optarg != NULL)
                {
                    if (isdigit (*optarg) == 0 ||
                        atoi (optarg) < 0 || atoi (optarg) > 4)
                    {
                        errx (EXIT_FAILURE,
                              "Please select tactic in [0,..,4].\n");
                    }

                    black_ai = atoi (optarg);
                }

                break;

            case 'w' :
                if (optarg != NULL)
                {
                    if (isdigit (*optarg) == 0 ||
                        atoi (optarg) < 0 || atoi (optarg) > 4)
                    {
                        errx (EXIT_FAILURE,
                              "Please select tactic in [0,..,4].\n");
                    }

                    white_ai = atoi (optarg);
                }

                break;

            case 's' :
                if (atoi (optarg) <= 0 || !board_cheak_size (atoi (optarg) * 2))
                {
                    errx (EXIT_FAILURE,
                          "Please select a size between %d and %d.\n",
                           MIN_BOARD_SIZE / 2, MAX_BOARD_SIZE / 2);
                }

                board_size = atoi (optarg) * 2;

                break;

            case 'c' :
                contest_mode = true;

                if (optarg != NULL)
                {
                    if (isdigit (*optarg) == 0 ||
                        atoi (optarg) < 0 || atoi (optarg) > 4)
                    {
                        errx (EXIT_FAILURE, "Please select AI in [1,..,4].\n");
                    }

                    contest_ai = atoi (optarg);
                }

                break;

            case 'a' :
                all = true;

                break;

            case 'v' :
                verbose = true;
                set_verbose ();

                break;

            case 'V' :
                version ();

                return EXIT_SUCCESS;

            case 'h' :
                help ();

                return EXIT_SUCCESS;

            default :
                errx (EXIT_FAILURE,
                      "Try 'reversi --help' for more information.\n");
        }
    }

    board_t *board = NULL;
    int i = optind;
    bool error = false;

    if (i == argc) /* If no file in argument. */
    {
        if (contest_mode)
        {
            errx (EXIT_FAILURE, "The contest mode need a file.\n");
        }

        if ((board = board_init (board_size)) == NULL)
        {
            errx (EXIT_FAILURE, "Impossible to init board of size %ld:"
                                 " memory allocation failed.\n", board_size);
        }

        game (player_used[black_ai], player_used[white_ai], board);

        if (board_print (board, stdout) == -1)
        {
            board_free (board);

            errx (EXIT_FAILURE, "Impossible to print board.\n");
        }

        board_free (board);
        printf ("Thanks for playing, see you soon!\n");
    }
    else
    {
        int max = (all) ? argc : i + 1;

        for (int j = i; j < max; j++)
        {
            if (contest_mode)
            {
                /* Find the next move to play with player select. */
                /* Recover name file and parse it to contest. */
                board = file_parser (argv[j]);

                if (board == NULL)
                {
                    error = true;
                    warnx ("Impossible to parse the file %s.\n", argv[j]);
                    board_free (board);

                    continue;
                }

                if (board_count_player_moves (board) == 0) /**/
                {
                    printf ("No move possible.\n\n");
                    board_free (board);

                    continue;
                }

                /* Move compute by the AI select. */
                move_t move_proposed = player_used[contest_ai] (board);
                char letter_column = move_proposed.column + 'a';
                char number_row[2];
                number_row[0]= move_proposed.row + '1';
                number_row[1]= '\0';
                char *row = (move_proposed.row == 9) ? "10" : number_row;

                if (verbose && contest_mode)
                {
                    printf ("\033[A\33[2K\033[A\33[2K\nThe %s proposed this "
                            "move: %c%s\n\n", char_player_used[3],
                            letter_column, row);
                }
                else
                {
                    printf ("%c%s\n",letter_column, row);
                }

                board_free (board);
            }
            else
            {
                board = file_parser (argv[j]);

                if (board == NULL)
                {
                    error = true;
                    warnx ("Impossible to parse file %s.\n", argv[j]);
                    board_free (board);

                    continue;
                }

                game (player_used[black_ai], player_used[white_ai], board);

                if (board_print (board, stdout) == -1)
                {
                    error = true;
                    warnx ("Impossible to print the board.\n");
                    board_free (board);

                    continue;
                }

                printf ("Thanks for playing, see you soon!\n");
                board_free (board);
            }
        }
    }

    return (error) ? EXIT_FAILURE : EXIT_SUCCESS;
}

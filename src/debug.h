/*
 * xiffview by r. eberle
 *
 * distributed under the terms of
 * AWeb Public License Version 1.0
 * https://www.yvonrozijn.nl/aweb/apl.txt
 *
 * empty placeholder debug functions:
*/

#ifdef DEBUG
#   define debugmsg(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");
#   define debugerr(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#else
#   define debugmsg(...) {}
#   define debugerr(...) {}
#endif

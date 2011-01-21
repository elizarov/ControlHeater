#ifndef PARSE_H_
#define PARSE_H_

#include <WProgram.h>

#define CMD_DUMP_STATE  '?'
#define CMD_DUMP_CONFIG 'C'

/**
 * This function returns '?', 'C' or digits from '1' to '4' if it parsed
 * the corresponding command in the serial input stream. The result
 * is zero if there are no more characters in the serial input.
 */
char parseCommand();

#endif /* PARSE_H_ */

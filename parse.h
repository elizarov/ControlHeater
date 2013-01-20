#ifndef PARSE_H_
#define PARSE_H_

const char CMD_DUMP_STATE  = '?';
const char CMD_DUMP_CONFIG = 'C';
const char CMD_DUMP_ZONES  = 'Z';

/**
 * This function returns '?', 'C' or digits from '1' to '4' if it parsed
 * the corresponding command in the serial input stream. The result
 * is zero if there are no more characters in the serial input.
 */
char parseCommand();

#endif /* PARSE_H_ */

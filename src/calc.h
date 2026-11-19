#ifndef FLARE_CALC_H
#define FLARE_CALC_H

#include <stdbool.h>

// Evaluates a mathematical expression
// Returns true if query is a valid math expression, stores evaluated numeric value in *out_result,
// and formatted result string in out_str.
bool Calc_Evaluate(const char *expr, double *out_result, char *out_str, int max_len);

// Copies text to the Windows Clipboard
bool Calc_CopyToClipboard(const char *text);

#endif // FLARE_CALC_H

// -*- mode: c++ -*- .
#ifndef _FLA_PARSER_H
#define _FLA_PARSER_H
#include "tm.h"
#include "utils.h"
using std::ptrdiff_t;
struct Location {
    ptrdiff_t off, line, col;
    void advance(bool newline);
    Location();
    Location(const Location &l);
};
struct ParseError {
    string msg;
    optional<Location> _loc;
    operator string() const;
};

Either<ParseError, Tm> parseTm(string);
#endif

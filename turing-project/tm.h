// -*- mode: c++ -*- .
#ifndef _FLA_TM_H
#define _FLA_TM_H

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
using std::int32_t, std::optional, std::string, std::vector, std::deque,
    std::unordered_set, std::unordered_map, std::pair;
using StateIdx = uint32_t;
using StateName = string;

struct TmBuilderError {
    string msg;
};

struct TmError {
    string msg;
};

enum Dir { L, R, N };

struct TapeChar {
    enum { Char, Blank, Wildcard } type;
    char c;
};

class Id {
  private:
    uint32_t _tapeCount;
    StateIdx _state;
    vector<int32_t> _position;
    vector<deque<char>> _tapeL, _tapeGE;
    char _blankChar;

  public:
    // fields
    StateIdx state() const;
    void state(StateIdx);
    int32_t position(uint32_t) const;
    uint32_t tapeCount() const;
    //
    Id(StateIdx, uint32_t, char, string);
    vector<char> get() const;
    char get(uint32_t, int32_t) const;
    string slice(uint32_t, int32_t, int32_t) const;
    string visibleSlice(uint32_t) const;
    pair<int32_t, int32_t> nonBlankRange(uint32_t) const;
    pair<int32_t, int32_t> visibleRange(uint32_t) const;
    void put(const vector<TapeChar> &);
    void move(const vector<Dir> &);
    string contents(uint32_t) const;
};

template <class S> struct Rule {
    S src, dst;
    vector<TapeChar> get, put;
    vector<Dir> dirs;
    Rule(S src, S dst, vector<TapeChar> get, vector<TapeChar> put,
         vector<Dir> dirs)
        : src(src), dst(dst), get(get), put(put), dirs(dirs) {}
};

class Tm;
class TmBuilder {
  private:
    uint32_t _tapeCount;
    unordered_set<StateName> _states, _finalStates;
    optional<StateName> _initialState;
    unordered_set<char> _inAlphabet, _tapeAlphabet;
    vector<Rule<StateName>> _rules;
    optional<char> _blankChar;
    TmBuilder(uint32_t);
    void _mustHaveState(const StateName &name) const;

  public:
    static TmBuilder withTapes(uint32_t);
    TmBuilder &addState(StateName);
    TmBuilder &addInputSymbol(char);
    TmBuilder &addTapeSymbol(char);
    TmBuilder &addTransition(StateName srcState, std::vector<TapeChar> &get,
                             StateName dstState, std::vector<TapeChar> &put,
                             const vector<Dir> &dir);
    TmBuilder &makeInitial(StateName);
    TmBuilder &makeFinal(StateName);
    TmBuilder &setBlankChar(char);
    Tm build() const;
};

class Tm {
    friend Tm TmBuilder::build() const;

  private:
    uint32_t _tapeCount;
    vector<string> _stateName;
    unordered_map<StateName, StateIdx> _stateId;
    unordered_set<StateIdx> _states, _finalStates;
    StateIdx _initialState;
    unordered_set<char> _inAlphabet, _tapeAlphabet;
    vector<vector<Rule<StateIdx>>> _rules;
    char _blankChar;
    Tm();

  public:
    const vector<StateIdx> finalStates() const;
    string stateName(StateIdx) const;
    bool validate(char c) const;
    bool validate(string input) const;
    bool transition(Id &) const;
    char blankChar() const;
    Id initialId(string) const;
    // TODO: add some interface (visitor???) to export it as .tm file.
};
#endif

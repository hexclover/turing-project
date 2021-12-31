#include "tm.h"
#include <algorithm>
#include <iostream>

TmBuilder::TmBuilder(uint32_t tapeCount) : _tapeCount(tapeCount) {}

TmBuilder TmBuilder::withTapes(uint32_t tapeCount) {
    if (tapeCount <= 0)
        throw TmBuilderError{"Tape count must be positive"};
    return TmBuilder(tapeCount);
}

void TmBuilder::_mustHaveState(const StateName &name) const {
    auto it = _states.find(name);
    if (it == _states.end())
        throw TmBuilderError{string("Undeclared state: ") + name};
}

TmBuilder &TmBuilder::addState(StateName name) {
    _states.insert(name);
    return *this;
}

TmBuilder &TmBuilder::addInputSymbol(char c) {
    _inAlphabet.insert(c);
    return *this;
}

TmBuilder &TmBuilder::addTapeSymbol(char c) {
    _tapeAlphabet.insert(c);
    return *this;
}

TmBuilder &TmBuilder::makeFinal(StateName name) {
    _mustHaveState(name);
    _finalStates.insert(name);
    return *this;
}

TmBuilder &TmBuilder::makeInitial(StateName name) {
    _mustHaveState(name);
    _initialState = name;
    return *this;
}

TmBuilder &TmBuilder::addTransition(StateName srcState,
                                    std::vector<TapeChar> &get,
                                    StateName dstState,
                                    std::vector<TapeChar> &put,
                                    const vector<Dir> &dirs) {
    _mustHaveState(srcState);
    _mustHaveState(dstState);
    if (get.size() != _tapeCount || put.size() != _tapeCount) {
        throw TmBuilderError{
            string("Number of get/put symbols must equal that of tapes")};
    } else if (dirs.size() != _tapeCount) {
        throw TmBuilderError{
            string("Number of directions must equal that of tapes")};
    }
    for (auto c : get) {
        if (c.type == c.Char && this->_tapeAlphabet.count(c.c) == 0) {
            throw TmBuilderError{string("Character ") + c.c +
                                 " to read is outside the alphabet"};
        }
    }
    for (auto c : put) {
        if (c.type == c.Char && this->_tapeAlphabet.count(c.c) == 0) {
            throw TmBuilderError{string("Character ") + c.c +
                                 " to write is outside the alphabet"};
        }
    }
    _rules.emplace_back(srcState, dstState, get, put, dirs);
    return *this;
}

TmBuilder &TmBuilder::setBlankChar(char c) {
    this->_blankChar = c;
    return *this;
}

Tm TmBuilder::build() const {
    Tm res;
    // Check if the blank symbol is declared
    if (!_blankChar.has_value()) {
        throw TmBuilderError{"Blank symbol not specified"};
    }
    // Check if blank symbol is in the tape symbol set
    if (_tapeAlphabet.count(_blankChar.value()) == 0) {
        throw TmBuilderError{string("The blank symbol ") + _blankChar.value() +
                             " is not in the tape alphabet"};
    }
    // Check if the tape alphabet is a superset of the input alphabet
    for (auto c : _inAlphabet) {
        if (_tapeAlphabet.count(c) == 0) {
            throw TmBuilderError{string("The input symbol ") + c +
                                 " is not present in the tape alphabet"};
        }
    }
    res._tapeCount = _tapeCount;
    for (auto s : _states) {
        auto idx = res._stateName.size();
        res._stateId[s] = idx;
        res._states.insert(idx);
        res._stateName.push_back(s);
    }
    for (auto s : _finalStates) {
        res._finalStates.insert(res._stateId.at(s));
    }
    res._initialState = res._stateId.at(_initialState.value());
    res._inAlphabet = _inAlphabet;
    res._tapeAlphabet = _tapeAlphabet;
    res._blankChar = _blankChar.value();
    // transitions
    res._rules.reserve(_states.size());
    for (size_t _ = 0; _ < _states.size(); ++_) {
        res._rules.emplace_back();
    }
    for (auto r : _rules) {
        auto src = res._stateId.at(r.src), dst = res._stateId.at(r.dst);
        res._rules.at(src).emplace_back(src, dst, r.get, r.put, r.dirs);
    }
    return res;
}

Tm::Tm() {}

char Tm::blankChar() const { return _blankChar; }

string Tm::stateName(StateIdx id) const { return this->_stateName.at(id); }

bool Tm::validate(char c) const { return _inAlphabet.count(c) != 0; }

bool Tm::validate(string input) const {
    for (auto c : input)
        if (_inAlphabet.count(c) == 0)
            return false;
    return true;
}

Id Tm::initialId(string input) const {
    for (auto c : input) {
        if (_inAlphabet.count(c) == 0)
            throw TmError{string("Not a valid input symbol: ") + c};
    }
    return Id{_initialState, _tapeCount, _blankChar, input};
}

Id::Id(StateIdx state, uint32_t tapeCount, char blankChar, string input)
    : _tapeCount(tapeCount), _state(state),
      _position(vector<int32_t>(tapeCount, 0)),
      _tapeL({tapeCount, deque<char>()}), _tapeGE({tapeCount, deque<char>()}),
      _blankChar(blankChar) {
    auto &first = _tapeGE.at(0);
    first.insert(first.begin(), input.begin(), input.end());
}

uint32_t Id::tapeCount() const { return _tapeCount; }

int32_t Id::position(uint32_t tape) const { return _position.at(tape); }

StateIdx Id::state() const { return _state; }
void Id::state(StateIdx state) { _state = state; }

vector<char> Id::get() const {
    vector<char> res;
    for (size_t i = 0; i < _tapeCount; ++i) {
        res.push_back(this->get(i, _position[i]));
    }
    return res;
}

char Id::get(uint32_t N, int32_t pos) const {
    auto orig = _position.at(N);
    size_t k = pos < orig ? -1 - (pos - orig) : pos - orig;
    const auto &tape = pos < orig ? _tapeL[N] : _tapeGE[N];
    if (tape.size() > k)
        return tape[k];
    return _blankChar;
}

void Id::put(const vector<TapeChar> &s) {
    for (size_t i = 0; i < _position.size() && i < s.size(); ++i) {
        if (s[i].type == TapeChar::Wildcard)
            continue;
        char c = s[i].type == TapeChar::Blank ? _blankChar : s[i].c;
        auto &tape = _tapeGE[i];
        if (!tape.empty()) {
            if (c == _blankChar && tape.size() == 1) {
                tape.pop_front();
            } else {
                tape[0] = c;
            }
        } else if (c != _blankChar) {
            tape.push_front(c);
        }
    }
}

void Id::move(const vector<Dir> &dirs) {
    for (size_t i = 0; i < dirs.size() && i < _position.size(); ++i) {
        deque<char> *toPop = nullptr, *toPush = nullptr;
        if (dirs[i] == L) {
            toPop = &(_tapeL[i]);
            toPush = &(_tapeGE[i]);
            --_position[i];
        } else if (dirs[i] == R) {
            toPop = &(_tapeGE[i]);
            toPush = &(_tapeL[i]);
            ++_position[i];
        }
        if (toPop && toPush) {
            if (!toPop->empty()) {
                auto c = toPop->front();
                if (c != _blankChar || !toPush->empty())
                    toPush->push_front(c);
                toPop->pop_front();
            } else if (!toPush->empty()) {
                toPush->push_front(_blankChar);
            }
        }
    }
}

// Returns a smallest range [left, right) containing all non-blank symbols.
// That left == right means there is no non-blank symbol on tape N.
pair<int32_t, int32_t> Id::nonBlankRange(uint32_t N) const {
    auto bounds = visibleRange(N);
    while (bounds.first < bounds.second && get(N, bounds.first) == _blankChar)
        ++bounds.first;
    while (bounds.first < bounds.second &&
           get(N, bounds.second - 1) == _blankChar)
        --bounds.second;
    return bounds;
}

// Returns a smallest range [left, right) containing all non-blank positions
// as well as the position under the tape head.
// In all cases, right - left >= 1.
pair<int32_t, int32_t> Id::visibleRange(uint32_t N) const {
    int32_t orig = _position.at(N);
    int32_t left = orig - _tapeL[N].size(), right = orig + _tapeGE[N].size();
    if (right == orig)
        ++right;
    return {left, right};
}

string Id::slice(uint32_t N, int32_t lo, int32_t hi) const {
    string res;
    // TODO: maybe optimize this
    while (lo < hi)
        res.push_back(this->get(N, lo++));
    return res;
}

string Id::visibleSlice(uint32_t N) const {
    string res;
    res.append(_tapeL.at(N).crbegin(), _tapeL.at(N).crend());
    res.append(_tapeGE.at(N).begin(), _tapeGE.at(N).end());
    if (res.empty())
        res.push_back(_blankChar);
    return res;
}

string Id::contents(uint32_t N) const {
    auto bounds = this->nonBlankRange(N);
    return this->slice(N, bounds.first, bounds.second);
}

bool Tm::transition(Id &id) const {
    auto cur = id.state();
    if (_finalStates.find(cur) == _finalStates.end() && _rules.size() > cur) {
        auto rules = _rules.at(cur);
        auto get = id.get();
        for (auto r : rules) {
            if (get.size() != r.get.size())
                continue;
            bool matched = true;
            for (size_t i = 0; i < r.get.size(); ++i) {
                if (r.get[i].type != TapeChar::Wildcard &&
                    r.get[i].c != get[i]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                id.put(r.put);
                id.move(r.dirs);
                id.state(r.dst);
                return true;
            }
        }
    }
    // Halted!
    return false;
}

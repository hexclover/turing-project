#include "parser.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using std::ptrdiff_t, std::uint32_t, std::unordered_set, std::vector,
    std::tuple, std::optional;

void Location::advance(bool newline) {
    auto x = string("a");
    ++off;
    if (newline) {
        ++line;
        col = 1;
    } else {
        ++col;
    }
}

Location::Location() : off(0), line(1), col(1) {}

Location::Location(const Location &l) {
    off = l.off, line = l.line, col = l.col;
}

ParseError::operator string() const {
    std::ostringstream s;
    if (this->_loc) {
        s << "Line " << this->_loc.value().line << ", column "
          << this->_loc.value().col << " (offset " << this->_loc.value().off
          << "):";
    } else {
        s << "Unknown location";
    }
    s << '\n' << this->msg;
    return s.str();
}

class TmParser {
  private:
    string _text;
    ptrdiff_t _len;
    Location _loc;
    // tm components
    optional<unordered_set<string>> _Q, _F;
    optional<unordered_set<char>> _S, _G;
    using DRule =
        tuple<string, vector<char>, string, vector<char>, vector<Dir>>;
    vector<DRule> _delta;
    optional<string> _q0;
    optional<char> _B;
    optional<uint32_t> _N;
    constexpr static auto isStateChar = [](char c) -> bool {
        return std::isalnum(c) || c == '_';
    };
    constexpr static auto isInputChar = [](char c) -> bool {
        const static unordered_set<char> ill = {' ', ',', ';', '{',
                                                '}', '*', '_'};
        return std::isgraph(c) && ill.count(c) == 0;
    };
    constexpr static auto isTapeChar = [](char c) -> bool {
        return isInputChar(c) || c == '_' || c == '*';
    };

    void init(string text) {
        _text = text;
        _loc = {};
        _len = text.length();
        _Q.reset();
        _F.reset();
        _S.reset();
        _G.reset();
        _delta.clear();
    }

    void throw_(string msg) const { throw ParseError{msg, {_loc}}; }
    void throw_if(bool cond, string msg) const {
        if (cond)
            throw ParseError{msg, {_loc}};
    }
    bool isEof() const { return _loc.off >= (ptrdiff_t)_text.length(); }
    void advance() { _loc.advance(!isEof() && _text[_loc.off] == '\n'); }

    auto eofP() const {
        return [this]() {
            if (!isEof())
                throw_("Expected EOF");
        };
    }

    template <class A> auto tryP(std::function<A(void)> f) {
        return [this, f]() {
            auto oldloc = _loc;
            try {
                return Either<ParseError, A>::inr(f());
            } catch (ParseError e) {
                _loc = oldloc;
                return Either<ParseError, A>::inl(e);
            }
        };
    }

    auto peekP() const {
        return [this]() {
            if (isEof())
                throw_("Nothing to peek");
            return _text[_loc.off];
        };
    }
    auto getP() {
        return [this]() {
            if (isEof())
                throw_("Nothing to get");
            advance();
            return _text[_loc.off];
        };
    }

    auto charP(char c) {
        return [this, c]() {
            if (isEof())
                throw_(string("Got EOF, expecting ") + c);
            if (_text[_loc.off] != c)
                throw_(string("Got ") + _text[_loc.off] + ", expecting " + c);
            advance();
            return c;
        };
    }
    auto predP(std::function<bool(int)> pred) {
        return [pred, this]() {
            char c = peekP()();
            if (pred(c)) {
                advance();
                return c;
            }
            throw_(string("Unexpected ") + c);
            assert(0);
        };
    }
    auto anyP(string range) {
        return [range, this]() {
            char c = peekP()();
            if (range.find(c) != range.npos) {
                advance();
                return c;
            }
            throw_(string("Expecting any char in ") + range + " but got " + c);
            assert(0);
        };
    }
    auto skipWs() {
        return [this]() -> void {
            while (1) {
                if (auto o = tryP<char>(peekP())()) {
                    if (isspace(o.getR())) {
                        advance();
                    } else if (o.getR() == ';') {
                        advance();
                        while (auto _ = tryP<char>(
                                   predP([](int c) { return c != '\n'; }))())
                            ;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        };
    }

    auto exactP(string s) {
        return [this, s]() {
            auto oldloc = _loc;
            try {
                for (char c : s)
                    charP(c)();
            } catch (ParseError e) {
                _loc = oldloc;
                throw e;
            }
            return s;
        };
    }

    auto lookaheadP(string s) {
        return [this, s]() -> string {
            auto oldloc = _loc;
            try {
                for (char c : s)
                    charP(c)();
            } catch (ParseError e) {
                _loc = oldloc;
                throw e;
            }
            _loc = oldloc;
            return s;
        };
    }

    // Should not throw
    template <class A> auto manyP(std::function<A()> a) {
        return [this, a]() {
            vector<A> res;
            while (auto x = tryP(a)()) {
                res.push_back(x.getR());
            }
            return res;
        };
    }

    template <class A> auto someP(std::function<A()> a) {
        return [this, a]() {
            vector<A> res;
            res.push_back(a());
            auto as = manyP(a)();
            res.insert(res.end(), as.begin(), as.end());
            return res;
        };
    }
    auto stateNameP() {
        return [this]() -> string {
            try {
                auto v = someP<char>(predP(isStateChar))();
                return {v.begin(), v.end()};
            } catch (ParseError) {
                throw_("Expecting legal state name");
                assert(0);
            }
        };
    }
    auto lineStartP() const {
        return [this]() -> void {
            if (_loc.col != 1) {
                throw_("Should be at beginning of line");
            }
        };
    }
    auto defBeginP() {
        return [this]() -> string {
            skipWs()();
            lineStartP()();
            charP('#')();
            auto id = someP<char>(predP(isalnum))();
            exactP(" = ")();
            return {id.begin(), id.end()};
        };
    }
    void defBeginOfP(string s) {
        auto t = defBeginP()();
        if (s != t) {
            throw_(string("Expecting definition of ") + s + " but got " + t);
        }
    }
    // whitespace till line break or eof
    auto defEndP() {
        return [this]() -> void {
            bool commented = false;
            while (!isEof()) {
                auto c = peekP()();
                if (c == '\n') {
                    break;
                } else if ((commented |= (c == ';'))) {
                    ;
                } else if (!isspace(c)) {
                    throw_(string("Extra ") + c + " before newline");
                }
                advance();
            }
        };
    }
    template <class Atom> auto setP(std::function<Atom()> atom) {
        return [this, atom]() {
            auto oldloc = _loc;
            unordered_set<Atom> res;
            try {
                exactP("{")();
                res.insert(atom());
                while (auto s = tryP<Atom>([this, atom]() {
                           charP(',')();
                           return atom();
                       })()) {
                    res.insert(s.getR());
                }
                exactP("}")();
                return res;
            } catch (ParseError e) {
                _loc = oldloc;
                throw e;
            }
        };
    }

    auto qP() { return setP<string>(stateNameP()); }

    auto sP() { return setP<char>(predP(isInputChar)); }

    auto gP() { return setP<char>(predP(isTapeChar)); }

    auto q0P() { return stateNameP(); }

    auto bP() { return predP(isTapeChar); }

    auto fP() { return setP<string>(stateNameP()); }

    auto nP() {
        return [this]() {
            auto chars = someP<char>(predP(isdigit))();
            string s = {chars.begin(), chars.end()};
            bool error = false;
            int v;
            try {
                v = std::stoi(s);
            } catch (...) {
                error = true;
            }
            if (v <= 0) {
                error = true;
            }
            if (error) {
                throw_(string("Not a positive integer: ") + s);
            }
            return v;
        };
    }

    auto ruleP() {
        return [this]() {
            skipWs()();
            lineStartP()();
            auto curState = stateNameP()();
            someP<char>(predP(isblank))();
            auto getSymb = someP<char>(predP(isTapeChar))();
            someP<char>(predP(isblank))();
            auto putSymb = someP<char>(predP(isTapeChar))();
            someP<char>(predP(isblank))();
            auto dirs = someP<Dir>([this]() {
                auto c = anyP("lr*")();
                if (c == 'l') {
                    return L;
                } else if (c == 'r') {
                    return R;
                } else if (c == '*') {
                    return N;
                } else {
                    assert(0);
                }
            })();
            manyP<char>(predP(isblank))();
            auto nextState = stateNameP()();
            return std::make_tuple(curState, getSymb, nextState, putSymb, dirs);
        };
    }

    void run() {
        while (!isEof()) {
            if (auto def = tryP<string>(defBeginP())()) {
                auto s = def.getR();
                if (s == "Q") {
                    auto q = qP()();
                    throw_if(_Q.has_value(), "Redefinition of Q");
                    _Q = q;
                } else if (s == "S") {
                    auto s = sP()();
                    throw_if(_S.has_value(), "Redefinition of S");
                    _S = s;
                } else if (s == "G") {
                    auto g = gP()();
                    throw_if(_G.has_value(), "Redefinition of G");
                    _G = g;
                } else if (s == "q0") {
                    auto q0 = q0P()();
                    throw_if(_q0.has_value(), "Redefinition of q0");
                    _q0 = q0;
                } else if (s == "B") {
                    auto b = bP()();
                    throw_if(_B.has_value(), "Redefinition of B");
                    _B = b;
                } else if (s == "F") {
                    auto f = fP()();
                    throw_if(_F.has_value(), "Redefinition of F");
                    _F = f;
                } else if (s == "N") {
                    auto n = nP()();
                    throw_if(_N.has_value(), "Redefinition of N");
                    _N = n;
                } else {
                    throw_(string("Unknown component of TM: ") + s);
                }
                defEndP()();
            } else {
                auto d = tryP<DRule>(ruleP())();
                if (d) {
                    _delta.push_back(d.getR());
                } else {
                    try {
                        skipWs()();
                        eofP()();
                    } catch (ParseError) {
                        auto e = d.getL();
                        e.msg = "In parsing transition rule: " + e.msg;
                        throw e;
                    }
                }
                defEndP()();
            }
        }

        if (!_Q.has_value())
            throw_("No definition of Q");
        if (!_S.has_value())
            throw_("No definition of S");
        if (!_G.has_value())
            throw_("No definition of G");
        if (!_q0.has_value())
            throw_("No definition of q0");
        if (!_B.has_value())
            throw_("No definition of B");
        if (!_F.has_value())
            throw_("No definition of F");
        if (!_N.has_value())
            throw_("No definition of N");
    }

    TapeChar toTapeChar(char c) const {
        if (c == _B)
            return TapeChar{TapeChar::Blank, c};
        else if (c == '*')
            return TapeChar{TapeChar::Wildcard, c};
        return TapeChar{TapeChar::Char, c};
    }

  public:
    TmParser() {}

    Either<ParseError, Tm> parse(string text) {
        init(text);
        try {
            run();
        } catch (ParseError e) {
            return Either<ParseError, Tm>::inl(e);
        }
        // TODO: catch errors here
        try {
            auto builder = TmBuilder::withTapes(_N.value());
            for (auto q : _Q.value()) {
                builder.addState(q);
            }
            for (auto s : _S.value()) {
                builder.addInputSymbol(s);
            }
            for (auto g : _G.value()) {
                builder.addTapeSymbol(g);
            }
            builder.makeInitial(_q0.value());
            for (auto f : _F.value()) {
                builder.makeFinal(f);
            }
            builder.setBlankChar(_B.value());
            for (auto r : _delta) {
                auto curState = std::get<0>(r), nextState = std::get<2>(r);
                vector<TapeChar> getSymb, putSymb;
                std::transform(std::get<1>(r).begin(), std::get<1>(r).end(),
                               std::back_inserter(getSymb),
                               [this](char c) { return toTapeChar(c); });
                std::transform(std::get<3>(r).begin(), std::get<3>(r).end(),
                               std::back_inserter(putSymb),
                               [this](char c) { return toTapeChar(c); });
                auto dirs = std::get<4>(r);
                builder.addTransition(curState, getSymb, nextState, putSymb,
                                      dirs);
            }
            return Either<ParseError, Tm>::inr(builder.build());
        } catch (TmBuilderError e) {
            return Either<ParseError, Tm>::inl(
                ParseError{string("Error when building TM: ") + e.msg,
                           std::optional<Location>()});
        }
    }
};

Either<ParseError, Tm> parseTm(string text) { return TmParser().parse(text); }

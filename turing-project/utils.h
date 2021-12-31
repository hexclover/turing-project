// -*- mode: c++ -*- .
#ifndef _FLA_UTILS_H
#define _FLA_UTILS_H
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
using std::string, std::shared_ptr, std::vector, std::tuple;
string replicate(string, size_t);
vector<string> split(string, char);
enum FileError { RF_NOTFOUND, RF_PERM, RF_OTHER };

template <class L, class R> class Either {
  private:
    shared_ptr<L> _lv;
    shared_ptr<R> _rv;
    Either() {}

  public:
    // TODO: fix memory leak???
    static Either<L, R> inl(L l) {
        Either<L, R> res;
        res._lv = std::make_shared<L>(l);
        return res;
    }
    static Either<L, R> inr(R r) {
        Either<L, R> res;
        res._rv = std::make_shared<R>(r);
        return res;
    }
    bool isL() const { return _lv != nullptr; }
    bool isR() const { return _rv != nullptr; };
    operator bool() const {
        return this->isR();
    }
    const L &getL() const {
        assert(isL());
        return *_lv;
    }
    const R &getR() const {
        assert(isR());
        return *_rv;
    }
    template <class T> Either<L, T> fmap(std::function<T(const R &)>) const;
};
Either<FileError, string> readFile(string);
Either<FileError, string> saveToFile(string, string);
#endif

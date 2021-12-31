#!/bin/bash
function die {
    echo $@
    exit 1
}

function expect_eq {
    local tag
    [ $# != 2 ] && [ $# != 3 ] && die "Expected two or three arguments to expected_eq"
    tag=""
    [ $# == 3 ] && tag="$3: "
    [ "$1" != "$2" ] && die "${tag}Expected $1 but got $2"
}

function test_errors {
    echo "Testing syntax errors."
    for TM in ./tests/error*.tm; do
        expected="syntax error"
        actual=$(./turing "$TM" "" 2>&1 >/dev/null) && die "Expected false return value for $TM"
        expect_eq "$expected" "$actual"
    done
    echo "Testing invalid input."
    for s in "aabaaaB" "B" "XXXa" "   "; do
        expected="illegal input"
        actual=$(./turing ./tests/1.a.tm "$s" 2>&1 >/dev/null) && die "Expected false return value for $TM"
        expect_eq "$expected" "$actual"
    done
    for s in "a" "_" " "; do
        expected="illegal input"
        actual=$(./turing ./tests/2.a.tm "$s" 2>&1 >/dev/null) && die "Expected false return value for $TM"
        expect_eq "$expected" "$actual"
    done
    for s in "xyz" "ghi" "  " "_" "B"; do
        expected="illegal input"
        actual=$(./turing ./tests/2.b.tm "$s" 2>&1 >/dev/null) && die "Expected false return value for $TM"
        expect_eq "$expected" "$actual"
    done
    echo "Testing invalid command line arguments."
    for cmd in "./turing &> /dev/null" "./turing ./tests/error1.tm &> /dev/null" "./turing /dev/abcdefg '' &> /dev/null"; do
        eval "$cmd" && die "Expecting false return value for command $cmd!"
    done
}

function test_palindrome {
    echo "Testing palindrome."
    TM=./tests/palindrome_detector_2tapes.tm

    function is_palindrome() {
        [ ${#1} -le 1 ] || ( [ "${1:0:1}" == "${1: -1}" ] && is_palindrome "${1:1: -1}" )
    }

    function to_binary() {
        bc <<< "obase=2;${1}"
    }

    for ((i=0; i<=500; ++i)); do
        bin=$(to_binary $i)
        expected=$(is_palindrome "${bin}" && echo True || echo False)
        actual=$(./turing "${TM}" "${bin}")
        if [ "${expected}" != "${actual}" ]; then
            echo "Failed for ${bin}: expected '${expected}', got '${actual}'"
            exit 1
        fi
        echo -n .
    done
    echo

    echo "Palindrome tests passed."
}

function test_gcd {
    echo "Testing GCD."
    local TM=../programs/gcd.tm

    function replicate {
        for ((i=0; i<$1; ++i)); do
            echo -n $2
        done
    }

    function gcd {
        if [ $1 -eq 0 ]; then
            echo -n $2
        else
            echo -n $(gcd $(($2 % $1)) $1)
        fi
    }

    function doTest {
        act=$(./turing "$TM" $(replicate $1 1)0$(replicate $2 1))
        exp=$(replicate $(gcd $1 $2) 1)
        if [ "$act" != "$exp" ]; then
            echo "Wrong: $1 $2, expected $exp, got $act";
            exit 1;
        fi
    }

    echo "Small random tests."
    for ((i=1; i<=50; ++i)); do
        p=$((RANDOM % 100))
        q=$((RANDOM % 100))
        doTest $p $q
        doTest $q $p
        doTest 0 $p
        doTest $q 0
        echo -n '.'
    done
    echo

    echo "Large random tests."
    for ((i=1; i<=50; ++i)); do
        p=$((RANDOM % 512 + 227))
        q=$((RANDOM % 512 + 199))
        doTest $p $q
        doTest $q $p
        echo -n '.'
    done
    echo

    echo "Fixed tests."
    for i in 0 1 2 3 5 7 10 $(seq 12 18) 24 32 50 96 97 99 100 128 768 1013 1024; do
        echo -n $i
        for j in 0 1 2 3 5 7 10 $(seq 12 18) 24 32 50 96 97 99 128 250 432 512 768; do
            doTest $i $j
        done
        echo -n '.'
    done
    echo

    echo "GCD tests passed."
}

test_errors
test_gcd
test_palindrome
echo "All tests passed."

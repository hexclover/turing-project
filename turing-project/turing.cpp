#include "parser.h"
#include "tm.h"
#include "utils.h"
#include <algorithm>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <sstream>

static int print_help = 0;
static int verbose_mode = 0;
static const string app_name = "turing";
static string tm_path, input_str;

static const struct option long_options[] = {
    {"help", no_argument, &print_help, 1},
    {"verbose", no_argument, &verbose_mode, 1},
    {0, 0, 0, 0}};

void print_usage(std::ostream &s) {
    s << "usage: " << app_name
              << " [-v|--verbose] [-h|--help] <tm> <input>" << std::endl;
}

void die(string msg, int code = 1) {
    std::cerr << msg << std::endl;
    std::exit(code);
}

void parse_options(int argc, char **argv) {
    int c;
    do {
        c = getopt_long(argc, argv, "hv", long_options, NULL);
        switch (c) {
        case 0:
            break;
        case 'h':
            print_help = 1;
            break;
        case 'v':
            verbose_mode = 1;
            break;
        case '?':
            die(string("Unknown option: -") + argv[optind - 1]);
            break;
        }
    } while (c != -1);
    if (print_help) {
        print_usage(std::cout);
        exit(0);
    }
    if (optind + 2 != argc) {
        print_usage(std::cerr);
        switch (argc - optind) {
        case 0:
            die("Expecting tm");
            break;
        case 1:
            die("Expecting input");
            break;
        default:
            die(string("Extra option: ") + argv[optind + 2]);
            break;
        }
    } else {
        tm_path = argv[optind];
        input_str = argv[optind + 1];
    }
}

void printId(int step, const Tm &tm, const Id &id) {
    // TODO: alignment
    std::cout << "Step   : " << step << std::endl;
    for (size_t N = 0; N < id.tapeCount(); ++N) {
        const auto bounds = id.visibleRange(N);
        const auto curpos = id.position(N);
        std::ostringstream indss, tapess, headss;
        for (auto index = bounds.first; index < bounds.second; ++index) {
            const auto indstr = std::to_string(index < 0 ? -index : index);
            const auto chr = id.get(N, index);
            const auto width = indstr.length();
            indss << indstr << ' ';
            tapess << chr << string(width, ' ');
            headss << (index == curpos ? '^' : ' ') << string(width, ' ');
        }
        std::cout << "Index" << N << " : " << indss.str() << '\n';
        std::cout << "Tape" << N << "  : " << tapess.str() << '\n';
        std::cout << "Head" << N << "  : " << headss.str() << '\n';
    }
    std::cout << "State  : " << tm.stateName(id.state()) << '\n';
    std::cout << "---------------------------------------------" << std::endl;
}

void run_tm() {
    const auto res = readFile(tm_path);
    if (res.isL()) {
        switch (res.getL()) {
        case RF_NOTFOUND:
            die(string("File not found: ") + tm_path);
            break;
        case RF_PERM:
            die(string("Permission denied: ") + tm_path);
            break;
        case RF_OTHER:
            die(string("Unknown error when reading ") + tm_path);
            break;
        }
    }
    const auto content = res.getR();
#ifdef DEBUG
    // std::cout << "Tm file content:\n" << content << std::endl;
#endif
    const auto parseResult = parseTm(content);
    if (parseResult.isL()) {
        std::cerr << "syntax error" << std::endl;
        if (verbose_mode) {
            std::cerr << (string)parseResult.getL() << std::endl;
        }
        exit(1);
    }

    // TODO: catch errors
    const auto tm = parseResult.getR();
    for (size_t i = 0; i < input_str.length(); ++i) {
        if (!tm.validate(input_str[i])) {
            if (verbose_mode) {
                std::cerr << "Input: " << input_str << std::endl;
                std::cerr << "==================== ERR ====================\n"
                          << "error: '" << input_str[i]
                          << "' was not declared in the set of input symbols\n"
                          << "Input: " << input_str << '\n'
                          << string(7 + i, ' ') << "^\n"
                          << "==================== END ===================="
                          << std::endl;
                exit(1);
            } else {
                die("illegal input");
            }
        }
    }
    if (verbose_mode) {
        std::cout << "Input: " << input_str << '\n';
        std::cout << "==================== RUN ===================="
                  << std::endl;
    }
    auto id = tm.initialId(input_str);
    uint32_t step = 0;
    while (1) {
        if (verbose_mode) {
            printId(step, tm, id);
        }
        if (!tm.transition(id)) {
            // Halt
            break;
        }
        ++step;
    };

    const auto contents = id.contents(0);
    if (verbose_mode) {
        std::cout << "Result: " << contents
                  << "\n==================== END ===================="
                  << std::endl;
    } else {
        std::cout << contents << std::endl;
    }
}

#ifdef DEBUG
void debug_warning_exit() {
    std::cerr << "\033[1;31mWARNING AGAIN: DEBUG BUILD; DO NOT SUBMIT\033[0m\n"
              << std::endl;
}
#endif

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
#ifdef DEBUG
    std::cerr << "\033[1;31mWARNING: DEBUG BUILD; DO NOT SUBMIT\033[0m\n"
              << std::endl;
    std::atexit(debug_warning_exit);
#endif
    parse_options(argc, argv);
#ifdef DEBUG
    std::cout << "Tm path: " << tm_path << "\nInput string: " << input_str
              << "\nVerbose: " << verbose_mode << "\nHelp: " << print_help
              << std::endl;
#endif
    run_tm();
}

#include "../subsequential_transducer.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

int checks = 0;
int failures = 0;

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << "\n";
    }
}

void check_eq(const std::string& actual, const std::string& expected, const std::string& message) {
    check(actual == expected, message + " expected=[" + expected + "] actual=[" + actual + "]");
}

struct Case {
    std::string name;
    std::string regex;
    std::map<std::string, std::string> accepted;
    std::vector<std::string> rejected;
};

void run_case(const Case& tc) {
    std::cout << "case: " << tc.name << "\n";

    Transducer raw;
    raw.from_regex(tc.regex);

    RealTimeTransducer realtime(raw);
    SubsequentialTransducer subsequential(realtime);

    const size_t before_states = subsequential.states_count();
    const size_t before_transitions = subsequential.get_transitions_count();

    SubsequentialTransducer minimal = subsequential.minimize();

    check(minimal.states_count() <= before_states, tc.name + " minimization does not increase states");
    check(minimal.get_transitions_count() <= before_transitions, tc.name + " minimization does not increase transitions");

    for (const auto& [input, output] : tc.accepted) {
        check_eq(realtime.traverse(input), output, tc.name + " realtime accepts " + input);
        check_eq(subsequential.traverse(input), output, tc.name + " subsequential accepts " + input);
        check_eq(minimal.traverse(input), output, tc.name + " minimal accepts " + input);
    }

    for (const std::string& input : tc.rejected) {
        check_eq(realtime.traverse(input), "Invalid input", tc.name + " realtime rejects " + input);
        check_eq(subsequential.traverse(input), "Invalid input", tc.name + " subsequential rejects " + input);
        check_eq(minimal.traverse(input), "Invalid input", tc.name + " minimal rejects " + input);
    }

    const size_t min_states = minimal.states_count();
    const size_t min_transitions = minimal.get_transitions_count();
    SubsequentialTransducer minimal_again = minimal.minimize();

    check(minimal_again.states_count() == min_states, tc.name + " second minimization keeps state count");
    check(minimal_again.get_transitions_count() == min_transitions, tc.name + " second minimization keeps transition count");
}

} // namespace

int main() {
    std::vector<Case> cases = {
        {
            "single pair",
            "\"a\"^\"x\"",
            {{"a", "x"}},
            {"", "b", "aa"}
        },
        {
            "union",
            "\"a\"^\"x\"|\"b\"^\"y\"",
            {{"a", "x"}, {"b", "y"}},
            {"", "ab", "c"}
        },
        {
            "concatenation",
            "\"a\"^\"x\".\"b\"^\"y\"",
            {{"ab", "xy"}},
            {"", "a", "b", "abc"}
        },
        {
            "common input and output prefix",
            "\"ab\"^\"xy\"|\"ac\"^\"xz\"",
            {{"ab", "xy"}, {"ac", "xz"}},
            {"", "a", "ad", "abc"}
        },
        {
            "input word is prefix of another input word",
            "\"ab\"^\"x\"|\"abc\"^\"xy\"",
            {{"ab", "x"}, {"abc", "xy"}},
            {"", "a", "ac", "abcd"}
        },
        {
            "empty output branch",
            "\"a\"^\"\"|\"b\"^\"bee\"",
            {{"a", ""}, {"b", "bee"}},
            {"", "ab", "c"}
        },
        {
            "parenthesized union before concatenation",
            "(\"a\"^\"x\"|\"b\"^\"y\").\"c\"^\"z\"",
            {{"ac", "xz"}, {"bc", "yz"}},
            {"", "a", "b", "c", "abc"}
        }
    };

    for (const Case& tc : cases) {
        run_case(tc);
    }

    std::cout << "checks=" << checks << " failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}


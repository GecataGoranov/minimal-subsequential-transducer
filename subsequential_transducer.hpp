#ifndef SUBSEQUENTIAL_TRANSDUCER_HPP
#define SUBSEQUENTIAL_TRANSDUCER_HPP

#include "real_time_transducer.hpp"
#include <set>
#include <queue>
#include <unordered_map>
#include <unordered_set>


using StateId = u_int32_t;

class DeterminedColoredAutomaton;

struct NewTransitionHash{
    size_t operator()(const std::pair<StateId, char>& key) const noexcept{
        return key.first;
    }
};


struct SubseqTransition{
    using Word = std::string;
    Word output;
    StateId target;

    bool for_deletion = false;
};


struct SubseqState{
    using Word = std::string;
    
    // StateLabel label;
    StateId id;
    bool is_final = false;
    Word final_output;
    std::unordered_map<char, SubseqTransition> transitions;
};


struct DenseSubseqState{
    StateId id;
    bool is_final = false;
    std::string final_output;
    std::vector<std::pair<char, SubseqTransition>> transitions;
};


class DenseSubsequentialTransducer{
    public:
        std::vector<DenseSubseqState> states;
        StateId initial;
        std::string initial_output;
        std::vector<char> alphabet;

        std::vector<StateId> reverse_topo_sort_states() const;
        std::vector<std::string> get_msos() const;
        std::string find_lcp(const std::string& s1, const std::string& s2) const;
        std::string remove_prefix(const std::string& pref, const std::string& str) const;
        void cannonize();
        size_t states_count() const;
        StateId get_initial() const;
        const std::vector<DenseSubseqState>& get_states() const;
};


class SubsequentialTransducer : public RealTimeTransducer{
    private:
        using NewTransSet = std::unordered_map<std::pair<StateId, char>, SubseqTransition, NewTransitionHash>;

        std::unordered_map<StateId, SubseqState> states;
        std::string initial_output = "";

        void from_realtime(const RealTimeTransducer& T);
        StateId create_empty_state();
        std::string find_lcp(const std::string& s1, const std::string& s2) const;
        std::string remove_prefix(const std::string& pref, const std::string& str) const;
        void trim();
        void sort_label(std::vector<std::pair<StateId, std::string>>& sl) const;
        std::unordered_map<StateId, bool> get_reachable() const;
        std::unordered_map<StateId, bool> get_co_reachable() const;
        void remove_unneeded_transitions(std::unordered_map<char, SubseqTransition>& transitions, std::vector<char>& to_remove);
        void remove_unneeded_states(const std::unordered_set<StateId>& st_to_remove);
        
        void from_minimal_automaton(const DeterminedColoredAutomaton& A);
        std::unordered_map<StateId, std::string> get_msos() const;
        std::vector<StateId> reverse_topo_sort_states() const;

        DenseSubsequentialTransducer to_dense() const;

    public:
        size_t states_count() const;
        SubsequentialTransducer(const DeterminedColoredAutomaton& A);
        SubsequentialTransducer(const RealTimeTransducer& T){
            this->from_realtime(T);
        }
        const std::unordered_map<StateId, SubseqState>& get_states() const;
        void cannonize();
        SubsequentialTransducer minimize();
        std::string traverse(std::string input);
        StateId get_initial() const;
        size_t get_transitions_count() const;
        size_t get_final_states_count() const;
};


#endif
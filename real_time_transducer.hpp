#ifndef REAL_TIME_TRANSDUCER_HPP
#define REAL_TIME_TRANSDUCER_HPP

#include "transducer.hpp"
#include <unordered_map>


class RealTimeTransducer : public Transducer{
    private:
        std::string eps_output = "";

        std::unordered_map<StateId, State> states;

        void turn_into_letter(Transducer& T);
        void convert_to_real_time();
        void create_letter_transitions(const Transition& orig_trans, StateId input_st);
        void insert_hollow_states(const std::vector<State>& states);
        std::vector<StateId> reverse_topo_sort_states() const;
        std::pair<std::string, bool> recursive_traverse(const std::string& input, const State& q, size_t i);
        std::unordered_map<StateId, std::unordered_map<StateId, std::string>> get_eps_closure(const std::vector<StateId>& reverse_topo_sorted);
        void trim();
        std::unordered_map<StateId, bool> get_reachable() const;
        std::unordered_map<StateId, bool> get_co_reachable() const;
        StateId create_empty_state();
        void remove_unneeded_states(const std::unordered_set<StateId>& st_to_remove);

        
    public:
        RealTimeTransducer(){}
        RealTimeTransducer(Transducer& T){
            this->turn_into_letter(T);
            this->convert_to_real_time();
        }
        // bool has_eps_transitions();
        std::string traverse(const std::string& input);
        unsigned states_count() const;
        const std::vector<State> get_states() const;
        const State& get_state_at(const StateId& a_st) const;
};


#endif
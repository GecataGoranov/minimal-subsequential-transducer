#ifndef TRANSDUCER_HPP
#define TRANSDUCER_HPP

#include <vector>
#include <string>
#include <set>
#include <stdexcept>
#include <unordered_set>


using StateId = u_int32_t;

struct Transition{
    using Word = std::string;
    Word input;
    Word output;
    StateId target;

    bool for_deletion = false;

    bool operator==(const Transition& other) const{
        return this->input == other.input &&
            this->output == other.output &&
            this->target == other.target;
    }
};

struct TransitionHash{
    StateId operator()(const Transition& tr) const{
       return tr.target;
    }
};


struct State{
    using Word = std::string;

    StateId id;
    bool is_final = false;
    Word final_output;
    std::vector<Transition> transitions;
};




class Transducer{
    private:
        std::vector<State> states;
    protected:
        

        using Word = std::string;

        StateId initial;
        std::unordered_set<char> alphabet;

        StateId create_empty_state();

    private:
        void from_pair(const std::pair<std::string, std::string> pair); // Expects the pair to be in EXACTLY this format: <x:y>, where x,y are words
        void from_concat(const Transducer* T1, Transducer* T2);
        void from_union(Transducer* T1, Transducer* T2);

        
        std::pair<std::string, std::string> atom_to_pair(const std::string atom) const;
        

        void add_offset(const unsigned offset);

    public:
        Transducer() = default;
        Transducer(const std::string mode, Transducer* T1 = NULL, Transducer* T2 = NULL, const std::string atom = ""){
            if (mode == "concat"){
                if(T1 == NULL || T2 == NULL){
                    throw std::invalid_argument("One of the transducers does not exist!");
                }
                this->from_concat(T1, T2);
            } else if(mode == "union"){
                if(T1 == NULL || T2 == NULL){
                    throw std::invalid_argument("One of the transducers does not exist!");
                }
                this->from_union(T1, T2);
            } else {
                if(T1 != NULL || T2 != NULL){
                    throw std::invalid_argument("Invalid mode!");
                } else {
                    if(atom == ""){
                        StateId state_id = this->create_empty_state();
                        this->initial = state_id;
                    } else {
                        std::pair<std::string, std::string> pair = this->atom_to_pair(atom);
                        this->from_pair(pair);
                    }
                }
            }
        }
        void from_regex(const std::string regex);
        const std::vector<State>& get_states() const;
        StateId get_initial() const;
        std::unordered_set<char> get_alphabet() const;
        State get_state_at(const StateId id) const;
        std::vector<std::string> reverse_polish_notation(const std::string regex) const;
        bool is_atom(const std::string expr) const;
        unsigned states_count() const;
};




#endif
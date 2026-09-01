#include <stdexcept>
#include <stack>
#include <algorithm>
#include <unordered_set>
#include "transducer.hpp"
#include <iostream>


unsigned Transducer::states_count() const{
    return this->states.size();
}

std::unordered_set<char> Transducer::get_alphabet() const{
    return this->alphabet;
}

State Transducer::get_state_at(const StateId id) const{
    return this->states[id];
}

bool Transducer::is_atom(const std::string expr) const{
    if(expr.size() < 5 || expr[0] != '"' || expr[expr.size()-1] != '"') return false;
    for(size_t i = 1; i < expr.size()-2; ++i){
        if(expr[i] == '"' && expr[i+1] == '^' && expr[i+2] == '"'){
            for(size_t j = i+3; j < expr.size()-1; ++j){
                if((expr[j] == '^' || expr[j] == '"') && expr[j-1] != '\\') return false;
            }
            return true;
        }        
    }
    return false;
}


std::pair<std::string, std::string> Transducer::atom_to_pair(const std::string atom) const{
    std::pair<std::string, std::string> output; 

    for(size_t i = 1; i < atom.size(); ++i){
        if(atom[i] == '^' && atom[i-1] != '\\'){
            output.first = atom.substr(1, i - 2);
            output.second = atom.substr(i + 2, atom.size() - i - 3);
        }
    }
    return output;
}

StateId Transducer::create_empty_state(){
    State st;
    st.id = this->states_count();
    this->states.push_back(st);

    return st.id;
}


std::vector<std::string> Transducer::reverse_polish_notation(const std::string regex) const{
    std::vector<std::string> operators;
    // Масив от атоми и оператори, където ще определим обратния полски запис
    std::vector<std::string> output;

    for(unsigned i = 0; i < regex.size(); ++i){
        if(regex[i] == ' ') continue;
        // Хващаме наредена двойка (малко грозно с тези булеви променливи)
        if((regex[i] == '"' && i == 0) || (regex[i] == '"' && regex[i-1] != '\\')){
            bool second_kavichki = false, hat = false, third_kavichki = false, fourth_kavichki = false;
            for(unsigned j=i+1; j < regex.size(); ++j){
                if(regex[j] == '"' && regex[j-1] != '\\' && !second_kavichki){
                    second_kavichki = true;
                    continue;
                }
                if(regex[j] == '^' && regex[j-1] != '\\' && second_kavichki && !hat){
                    hat = true;
                    continue;
                }
                if(regex[j] == '"' && regex[j-1] != '\\' && second_kavichki && hat && !third_kavichki){
                    third_kavichki = true;
                    continue;
                }
                if(regex[j] == '"' && regex[j-1] != '\\' && second_kavichki && hat && third_kavichki && !fourth_kavichki){
                    fourth_kavichki = true;
                }
                if(fourth_kavichki){
                    std::string atom = regex.substr(i, j - i + 1);
                    output.push_back(atom);
                    i = j;
                    break;
                }
            }
        } else if(i > 0 && regex[i] == '|' && regex[i-1] != '\\'){
            while(!operators.empty() && operators.back() != "("){
                output.push_back(operators.back());
                operators.pop_back();
            }
            operators.push_back(std::string(1, regex[i]));
        } else if(i > 0 && regex[i] == '.' && regex[i-1] != '\\'){
            while(!operators.empty() && operators.back() == "."){
                output.push_back(operators.back());
                operators.pop_back();
            }
            operators.push_back(std::string(1, regex[i]));
        } else if((regex[i] == '(' && i == 0) || (regex[i] == '(' && regex[i-1] != '\\')){
            operators.push_back(std::string(1, regex[i]));
        } else if(regex[i] == ')' && regex[i-1] != '\\'){
            while(!operators.empty() && operators.back() != "("){
                output.push_back(operators.back());
                operators.pop_back();
            }
            if(operators.empty()){
                throw std::invalid_argument("Mismatching parenthesis");
            }
            operators.pop_back();
        }
    }

    while(!operators.empty()){
        if(operators.back() == ")"){
            throw std::invalid_argument("Mismatching parenthesis");
        }
        output.push_back(operators.back());
        operators.pop_back();
    }

    return output;
}

void Transducer::from_regex(const std::string regex){
    std::vector<std::string> tokens = this->reverse_polish_notation(regex);
    std::stack<Transducer> tr_stack;
    for(auto token:tokens){
        if(this->is_atom(token)){
            tr_stack.emplace("atom", nullptr, nullptr, token);
        } else if(token == "."){
            Transducer T1 = std::move(tr_stack.top());
            tr_stack.pop();
            Transducer T2 = std::move(tr_stack.top());
            tr_stack.pop();
            tr_stack.emplace("concat", &T2, &T1, "");
        } else if(token == "|"){
            Transducer T1 = std::move(tr_stack.top());
            tr_stack.pop();
            Transducer T2 = std::move(tr_stack.top());
            tr_stack.pop();
            tr_stack.emplace("union", &T1, &T2, "");
        } else {
            throw std::runtime_error("There is an error with the Shunting Yard!");
        }
    }
    if(tr_stack.size() != 1){
        throw std::runtime_error("Something is wrong!");
    }
    
    Transducer trans = std::move(tr_stack.top());
    this->states = std::move(trans.states);
    this->initial = std::move(trans.initial);
    this->alphabet = std::move(trans.alphabet);
}


// Строим пребразувател q--u/v-->p
void Transducer::from_pair(const std::pair<std::string, std::string> pair){
    StateId q_0_id = this->create_empty_state();
    StateId q_1_id = this->create_empty_state();

    State& q_0 = this->states[q_0_id];
    State& q_1 = this->states[q_1_id];
    
    Transition tr;
    tr.input = pair.first;
    tr.output = pair.second;
    tr.target = q_1.id;
    q_1.is_final = true;

    q_0.transitions.push_back(tr);

    this->initial = q_0.id;
    for(char c: pair.first){
        this->alphabet.insert(c);
    }
    for(char c: pair.second){
        this->alphabet.insert(c);
    }
}

void Transducer::add_offset(const unsigned offset){
    for(auto& st: this->states){
        st.id += offset;
        for(auto& tr: st.transitions){
            tr.target += offset;
        }
    }
    this->initial += offset;
}


void Transducer::from_concat(const Transducer* T1, Transducer* T2){
    T2->add_offset(T1->states_count());
    // Правим всяко финално състояние на T1 да спре да е финално и да сочи към T2->initial
    for(auto st: T1->states){
        if(st.is_final){
            Transition tr;
            tr.target = T2->initial;
            st.transitions.push_back(tr);
            st.is_final = false;
        }
        this->states.push_back(st);
    }
    
    // Залепяме всичко от T2 към T1
    this->initial = T1->initial;
    this->states.insert(this->states.end(), T2->states.begin(), T2->states.end());
    this->alphabet.insert(T1->alphabet.begin(), T1->alphabet.end());
    this->alphabet.insert(T2->alphabet.begin(), T2->alphabet.end());
}

// Обединяваме елементите на двата автомата, като слагаме ново начално състояние, което да сочи към началните на T1 и T2
void Transducer::from_union(Transducer* T1, Transducer* T2){
    T1->add_offset(1);
    T2->add_offset(T1->states_count() + 1);
    
    StateId st_id = create_empty_state();
    State& st = this->states[st_id];

    Transition tr1;
    tr1.target = T1->initial;
    st.transitions.push_back(tr1);
    
    Transition tr2;
    tr2.target = T2->initial;
    st.transitions.push_back(tr2);

    this->initial = st.id;
    this->states.insert(this->states.end(), T1->states.begin(), T1->states.end());
    this->states.insert(this->states.end(), T2->states.begin(), T2->states.end());

    this->alphabet.insert(T1->alphabet.begin(), T1->alphabet.end());
    this->alphabet.insert(T2->alphabet.begin(), T2->alphabet.end());
}


const std::vector<State>& Transducer::get_states() const{
    return this->states;
}

StateId Transducer::get_initial() const{
    return this->initial;
}
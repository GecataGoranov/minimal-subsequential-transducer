#include "real_time_transducer.hpp"
#include <queue>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <unordered_map>


void RealTimeTransducer::turn_into_letter(Transducer& T){
    const std::vector<State>& T_states = T.get_states();
    // Добавяме "кухи" състояния, съответстващи на състоянията на T
    this->insert_hollow_states(T_states);
    this->initial = T.get_initial();
    for(auto& st: T_states){
        for(unsigned i = 0; i < st.transitions.size(); ++i){
            Transition tr = st.transitions[i];
            if(tr.input.size() > 1){
                // Раздуваме входа на няколко ето така:
                // q --abc/xyz--> p
                // q --a/xyz--> q_0 --b/eps--> q_1 --c/eps--> p
                this->create_letter_transitions(tr, st.id);
            } else {
                this->states[st.id].transitions.push_back(tr);
            }
        }
    }
}


void RealTimeTransducer::create_letter_transitions(const Transition& orig_trans, StateId input_st){
    std::vector<StateId> new_states;
    for(unsigned i = 0; i < orig_trans.input.size()-1; ++i){
        StateId q_id = this->create_empty_state();
        new_states.push_back(q_id);
    }

    Transition initial_trans;
    initial_trans.input = orig_trans.input[0];
    initial_trans.output = orig_trans.output;
    initial_trans.target = new_states[0];
    this->states[input_st].transitions.push_back(initial_trans);

    for(unsigned i = 1; i < orig_trans.input.size() - 1; ++i){
        Transition tr;
        tr.input = orig_trans.input[i];
        tr.output = "";
        tr.target = new_states[i];
        this->states[new_states[i-1]].transitions.push_back(tr);
    }

    Transition last_tr;
    last_tr.output = "";
    last_tr.input = orig_trans.input.back();
    last_tr.target = orig_trans.target;
    this->states[new_states[new_states.size()-1]].transitions.push_back(last_tr);
}


void RealTimeTransducer::insert_hollow_states(const std::vector<State>& states){
    for(unsigned i = 0; i < states.size(); ++i){
        StateId new_state_id = this->create_empty_state();
        State& new_state = this->states[new_state_id];
        new_state.id = states[i].id;
        new_state.is_final = states[i].is_final;
        new_state.final_output = states[i].final_output;
    }
}


StateId RealTimeTransducer::create_empty_state(){
    State st;
    st.id = this->states_count();
    this->states[st.id] = st;
    return st.id;
}


unsigned RealTimeTransducer::states_count() const{
    return this->states.size();
}


std::vector<StateId> RealTimeTransducer::reverse_topo_sort_states() const{
    std::queue<State> q;
    std::vector<StateId> topo_sorted;
    std::unordered_map<StateId, unsigned> in_degree;

    for(auto& [st_id, st]: this->states){
        in_degree[st_id] = 0;
    }

    for(auto& [st_id, st]: this->states){
        for(auto& tr: st.transitions){
            ++in_degree[tr.target];
        }
    }
    
    for(unsigned i = 0; i < this->states_count(); ++i){
        if(in_degree[i] == 0){
            q.push(this->states.at(i));
        }
    }

    while(!q.empty()){
        State st = q.front();
        q.pop();

        for(auto& tr: st.transitions){
            --in_degree[tr.target];

            if(in_degree[tr.target] == 0){
                q.push(this->states.at(tr.target));
            }
        }
        topo_sorted.push_back(st.id);
    }
    std::reverse(topo_sorted.begin(), topo_sorted.end());
    return topo_sorted;
}



void RealTimeTransducer::convert_to_real_time(){
    std::vector<StateId> reverse_topo_sorted = this->reverse_topo_sort_states();
    std::unordered_map<StateId, std::unordered_map<StateId, std::string>> eps_closure = this->get_eps_closure(reverse_topo_sorted);
    std::vector<State> new_states;
    std::unordered_map<StateId, std::vector<Transition>> new_transitions;


    for(auto& [st_id, st]: this->states){
        // Пазим вече видените преходи от дадено състояние, за да избегнем повторения
        std::unordered_set<Transition, TransitionHash> seen;
        for(auto& [q_0_id, u]: eps_closure[st_id]){
            State q_0 = this->states[q_0_id];
            for(auto& tr: q_0.transitions){
                if(tr.input == "") continue;
                std::string v = tr.output;
                for(auto& [q_1_id, w]: eps_closure[tr.target]){
                    Transition new_tr;
                    new_tr.input = tr.input;
                    new_tr.output = u + v + w;
                    new_tr.target = q_1_id;
                    if(seen.find(new_tr) != seen.end()) continue;
                    new_transitions[st_id].push_back(new_tr);
                    seen.insert(new_tr);
                }
            }
        }
    }

    for(auto& [st_id, transitions]: new_transitions){
        State& st = this->states.at(st_id);
        st.transitions = std::move(transitions);
    }

    // Трием епсилон-преходите
    for(auto& [st_id, st]: this->states){
        st.transitions.erase(
            std::remove_if(
                st.transitions.begin(),
                st.transitions.end(),
                [](const Transition& tr) {
                    return tr.for_deletion;
                }
            ),
            st.transitions.end()
        );
    }
    this->trim();
}


void RealTimeTransducer::remove_unneeded_states(const std::unordered_set<StateId>& st_to_remove){
    for(StateId st: st_to_remove){
        this->states.erase(st);
    }
}


// Взимаме епсилон-затварянето
// Представяме го като речник, който за всяко състояние 
// ни дава речник със състояние и изход, до които можем
// да стигнем само с епсилон-преходи
// Всяко състояние участва в собственото си епсилон-затваряне
std::unordered_map<StateId, std::unordered_map<StateId, std::string>> RealTimeTransducer::get_eps_closure(const std::vector<StateId>& reverse_topo_sorted){
    std::unordered_map<StateId, std::unordered_map<StateId, std::string>> eps_closure;
    for(auto& q_id: reverse_topo_sorted){
        eps_closure[q_id][q_id] = "";
        State& q = this->states[q_id];
        for(auto& tr: q.transitions){
            if(tr.input == ""){
                tr.for_deletion = true;
                eps_closure[q_id][tr.target] = tr.output;
                for(auto& [p, u]: eps_closure[tr.target]){
                    eps_closure[q_id][p] = tr.output + u;
                }
            }
        }
    }
    return eps_closure;
}


std::string RealTimeTransducer::traverse(const std::string& input){
    std::string output = "";

    State q = this->states[this->initial];
    for(auto& tr: q.transitions){
        if(tr.input[0] == input[0]){
            std::pair<std::string, bool> pass = this->recursive_traverse(input, this->states[tr.target], 1);
            if(pass.second) return tr.output + pass.first;
        }
    }
    return "Invalid input";
}


std::pair<std::string, bool> RealTimeTransducer::recursive_traverse(const std::string& input, const State& q, size_t i){
    if(i == input.size() && q.is_final) return {"", true};
    if(i == input.size()) return {"", false};

    for(auto& tr: q.transitions){
        if(tr.input[0] == input[i]){
            std::pair<std::string, bool> res = this->recursive_traverse(input, this->states[tr.target], i+1);
            if(res.second){
                return {tr.output + res.first, true};
            }
        }
    }
    return {"", false};
}



void RealTimeTransducer::trim(){
    std::unordered_map<StateId, bool> reachable = this->get_reachable();
    std::unordered_map<StateId, bool> co_reachable = this->get_co_reachable();
    std::unordered_set<StateId> for_deletion;

    for(auto& [st_id, st]: this->states){
        if(!(reachable[st_id] && co_reachable[st_id])) for_deletion.insert(st_id);
    }

    this->remove_unneeded_states(for_deletion);
    for(auto& [st_id, st]: this->states){
        std::vector<unsigned> to_delete;
        for(size_t i = 0; i < st.transitions.size(); ++i){
            Transition& tr = st.transitions[i];
            if(for_deletion.contains(tr.target)){
                to_delete.push_back(i);
            }
        }
        std::sort(to_delete.begin(), to_delete.end(), std::greater<unsigned>());
        for(size_t i : to_delete){
            if(i < st.transitions.size()){
                st.transitions.erase(st.transitions.begin() + i);
            }
        }

    }
}


const std::vector<State> RealTimeTransducer::get_states() const{
    std::vector<State> res;
    for(auto& [st_id, st]: this->states){
        res.push_back(st);
    }
    return res;
}

const State& RealTimeTransducer::get_state_at(const StateId& a_st) const{
    return this->states.at(a_st);
}


std::unordered_map<StateId, bool> RealTimeTransducer::get_reachable() const{
    std::unordered_map<StateId, bool> reachable;

    for(auto& [st_id, st]: this->states){
        reachable[st.id] = false;
    }

    std::queue<StateId> q;
    reachable[this->initial] = true;
    q.push(this->initial);
    while(!q.empty()){
        StateId p_id = q.front();
        q.pop();
        State p = this->states.at(p_id);
        for(auto& tr: p.transitions){
            if(!reachable[tr.target]){
                reachable[tr.target] = true;
                q.push(tr.target);
            }
        }
    }
    return reachable;
}

std::unordered_map<StateId, bool> RealTimeTransducer::get_co_reachable() const{
    std::unordered_map<StateId, std::vector<StateId>> predecessors;
    std::unordered_map<StateId, bool> co_reachable;

    for (auto& [st_id, st] : this->states) {
        predecessors[st_id] = {};
        co_reachable[st_id] = false;
    }

    for(auto& [st_id, st]: this->states){
        for(auto& tr: st.transitions){
            predecessors[tr.target].push_back(st_id);
        }
    }

    std::queue<StateId> q;

    for (auto& [st_id, st] : this->states) {
        if (st.is_final) {
            co_reachable[st_id] = true;
            q.push(st_id);
        }
    }

    while (!q.empty()) {
        StateId current = q.front();
        q.pop();

        for (StateId pred : predecessors[current]) {
            if (!co_reachable[pred]) {
                co_reachable[pred] = true;
                q.push(pred);
            }
        }
    }

    return co_reachable;
}

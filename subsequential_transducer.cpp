#include "subsequential_transducer.hpp"
#include "new_automaton.hpp"
#include <unordered_set>
#include <queue>
#include <iostream>
#include <algorithm>


SubsequentialTransducer::SubsequentialTransducer(const DeterminedColoredAutomaton& A){
    this->from_minimal_automaton(A);
}

StateId SubsequentialTransducer::get_initial() const {
    return this->initial;
}

StateId DenseSubsequentialTransducer::get_initial() const {
    return this->initial;
}

size_t SubsequentialTransducer::states_count() const{
    return this->states.size();
}

size_t DenseSubsequentialTransducer::states_count() const{
    return this->states.size();
}

const std::unordered_map<StateId, SubseqState>& SubsequentialTransducer::get_states() const{
    return this->states;
}

const std::vector<DenseSubseqState>& DenseSubsequentialTransducer::get_states() const{
    return this->states;
}

StateId SubsequentialTransducer::create_empty_state(){
    SubseqState st;
    st.id = this->states_count();
    this->states[st.id] = st;
    return st.id;
}


std::string SubsequentialTransducer::remove_prefix(const std::string& pref, const std::string& str) const{
    size_t i = pref.size();
    if(pref == "") return str;
    if(str.substr(0,i) != pref) throw std::runtime_error("Something is wrong with the construction.");
    return str.substr(i);
}


std::string SubsequentialTransducer::find_lcp(const std::string& s1, const std::string& s2) const{
    std::string res = "";
    size_t minlen = std::min(s1.size(), s2.size());
    for(size_t i = 0; i < minlen; ++i){
        if(s1[i] == s2[i]) res += s1[i];
        else break;
    }
    return res;
}


void SubsequentialTransducer::trim(){
    std::unordered_map<StateId, bool> reachable = this->get_reachable();
    std::unordered_map<StateId, bool> co_reachable = this->get_co_reachable();
    std::unordered_set<StateId> for_deletion;

    for(const auto& [st_id, st]: this->states){
        if(!(reachable[st_id] && co_reachable[st_id])) for_deletion.insert(st_id);
    }

    this->remove_unneeded_states(for_deletion);
    for(auto& [st_id, st]: this->states){
        std::vector<char> to_delete;
        for(auto& [c, tr]: st.transitions){
            if(for_deletion.contains(tr.target)){
                to_delete.push_back(c);
            }
        }
        for(const char& c: to_delete){
            st.transitions.erase(c);
        }
    }
}


std::unordered_map<StateId, bool> SubsequentialTransducer::get_reachable() const{
    std::unordered_map<StateId, bool> reachable;

    for(const auto& [st_id, st]: this->states){
        reachable[st_id] = false;
    }

    std::queue<StateId> q;
    reachable[this->initial] = true;
    q.push(this->initial);
    while(!q.empty()){
        StateId p_id = q.front();
        q.pop();
        SubseqState p = this->states.at(p_id);
        for(auto& [c, tr]: p.transitions){
            if(!reachable[tr.target]){
                reachable[tr.target] = true;
                q.push(tr.target);
            }
        }
    }
    return reachable;
}

std::unordered_map<StateId, bool> SubsequentialTransducer::get_co_reachable() const{
    std::unordered_map<StateId, std::vector<StateId>> predecessors;
    std::unordered_map<StateId, bool> co_reachable;

    for (const auto& [st_id, st] : this->states) {
        predecessors[st_id] = {};
        co_reachable[st_id] = false;
    }

    for(const auto& [st_id, st]: this->states){
        for(auto& [c, tr]: st.transitions){
            predecessors[tr.target].push_back(st_id);
        }
    }

    std::queue<StateId> q;

    for (const auto& [st_id, st] : this->states) {
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


void SubsequentialTransducer::remove_unneeded_states(const std::unordered_set<StateId>& st_to_remove){
    for(StateId st: st_to_remove){
        this->states.erase(st);
    }
}


void SubsequentialTransducer::cannonize(){
    std::unordered_map<StateId, std::string> msos = this->get_msos();
    std::string init_out(msos[this->initial].begin(), msos[this->initial].end());
    this->initial_output = init_out;
    for(auto& [st_id, st]: this->states){
        for(auto& [c, tr]: st.transitions){
            std::string mso_st(msos[st_id].begin(), msos[st_id].end());
            std::string mso_target(msos[tr.target].begin(), msos[tr.target].end());
            tr.output = this->remove_prefix(mso_st, tr.output + mso_target);
        }
        if(st.is_final){
            std::string mso_f(msos[st_id].begin(), msos[st_id].end());
            st.final_output = this->remove_prefix(mso_f, st.final_output);
        }
    }
}


void SubsequentialTransducer::from_minimal_automaton(const DeterminedColoredAutomaton& A){
    std::unordered_map<StateId, StateId> old_to_new;
    const std::vector<DeterminedState>& a_states = A.get_states();
    
    for(const auto& st: a_states){
        StateId new_id = this->create_empty_state();
        SubseqState& new_state = this->states.at(new_id);
        old_to_new[st.id] = new_id;
        new_state.is_final = st.is_final;
        if (st.is_final) {
            new_state.final_output = A.get_final_output(st.final_output_id);
        }
    }
    this->initial = old_to_new.at(A.get_initial());

    for(const auto& st: a_states){
        SubseqState& q = this->states[old_to_new[st.id]];

        for(const auto& tr: st.transitions){
            SubseqTransition new_tr;
            std::string input = A.get_symbol(tr.symbol).first;
            std::string output = A.get_symbol(tr.symbol).second;

            new_tr.output = output;
            new_tr.target = old_to_new.at(tr.target);
            q.transitions[input[0]] = new_tr;
        }
    }
}

SubsequentialTransducer SubsequentialTransducer::minimize(){
    this->trim();
    std::cout << "Trimmed" << std::endl;
    DenseSubsequentialTransducer dense = this->to_dense();
    dense.cannonize();
    std::cout << "Turned into cannonical" << std::endl;


    DeterminedColoredAutomaton A(dense);
    DeterminedColoredAutomaton A_minimal = A.get_minimal();

    SubsequentialTransducer Minimal(A_minimal);
    Minimal.initial_output = dense.initial_output;
    Minimal.alphabet = this->alphabet;
    return Minimal;
}


std::string SubsequentialTransducer::traverse(std::string input){
    std::string output = this->initial_output;
    SubseqState q = this->states.at(this->initial);
    for(size_t i = 0; i < input.size(); ++i){
        if(q.transitions.find(input[i]) == q.transitions.end()){
            std::string invalid = "Invalid input";
            return invalid;
        }
        output += q.transitions[input[i]].output;
        q = this->states.at(q.transitions[input[i]].target);
    }
    if(!q.is_final){
        std::string invalid = "Invalid input";
        return invalid;
    }
    output += q.final_output;
    return output;
}


static void hash_combine(size_t& seed, size_t h) noexcept {
    seed ^= h + (seed << 6) + (seed >> 2);
}

struct PairHash {
    size_t operator()(const std::pair<StateId, std::string>& p) const noexcept {
        size_t seed = std::hash<StateId>{}(p.first);
        hash_combine(seed, std::hash<std::string>{}(p.second));
        return seed;
    }
};

using StateLabel = std::vector<std::pair<StateId, std::string>>;


struct StateLabelHash {
    static void hash_combine(size_t& seed, size_t h) noexcept {
        seed ^= h + (seed << 6) + (seed >> 2);

    }

    size_t operator()(const StateLabel& label) const noexcept {
        size_t seed = 0;
        hash_combine(seed, std::hash<size_t>{}(label.size()));

        for (const auto& [state, output] : label) {
            hash_combine(seed, std::hash<StateId>{}(state));
            hash_combine(seed, std::hash<std::string>{}(output));
        }

        return seed;
    }
};



void SubsequentialTransducer::from_realtime(const RealTimeTransducer& T){
    this->alphabet = T.get_alphabet();

    // По подадено множество {(q_i, w_i)} ни дава на кое състояние отговаря
    std::unordered_map<StateLabel, StateId, StateLabelHash> seen;
    // По подадено ID на състояние ни казва неговото съответно множество
    std::unordered_map<StateId, StateLabel> labels;
    StateLabel initial;

    // Пълним всичко с началното състояние
    initial.push_back({T.get_initial(), ""});
    this->initial = create_empty_state();
    seen[initial] = this->initial;
    labels[this->initial] = initial;

    // Правим опашка и първо слагаме началното състояние
    std::queue<StateId> q;
    q.push(this->initial);


    while(!q.empty()){
        StateId S_id = q.front();
        q.pop();
        const StateLabel& S = labels.at(S_id);

        // За всяка буква, за която има преход ни дава кандидат-множество (без още да сме му извадили lcp-то)
        std::unordered_map<char, StateLabel> candidates;
        // Пълним lcp за преходите за всяка буква итеративно
        std::unordered_map<char, std::string> lcps;

        for(const auto& [st_id, str]: S){
            // Взимаме състояние
            const State& rt_state = T.get_state_at(st_id);
            // За всеки негов преход
            for(const auto& tr: rt_state.transitions){
                // Трупаме за съответната буква кандидат-състоянието
                char input = tr.input[0];

                std::string candidate_output;
                candidate_output.reserve(str.size() + tr.output.size());
                candidate_output.append(str);
                candidate_output.append(tr.output);
                
                // Изчисляваме lcp за същата буква
                auto it = lcps.find(input);
                if(it == lcps.end()){
                    lcps.emplace(input, candidate_output);
                } else {
                    it->second = this->find_lcp(it->second, candidate_output);
                }
                candidates[input].push_back({tr.target,std::move(candidate_output)});
            }
        }

        // Нормализираме кандидат-състоянията с техните lcp
        for (auto& [c, sl] : candidates) {
            this->sort_label(sl);

            StateLabel normalized;
            normalized.reserve(sl.size());

            for (auto& [q_id, str] : sl) {
                normalized.push_back({q_id, this->remove_prefix(lcps.at(c), str)});
            }

            this->sort_label(normalized);
            sl = std::move(normalized);

            auto it = seen.find(sl);

            if (it == seen.end()) {
                StateId new_st_id = this->create_empty_state();
                SubseqState& new_st = this->states.at(new_st_id);

                for (auto& [q_id, str] : sl) {
                    if (T.get_state_at(q_id).is_final) {
                        new_st.is_final = true;
                        new_st.final_output = str;
                        break;
                    }
                }

                labels.emplace(new_st_id, sl);
                it = seen.emplace(std::move(sl), new_st_id).first;
                q.push(new_st_id);
            }

            SubseqState& st = this->states.at(S_id);

            SubseqTransition new_tr;
            new_tr.output = lcps.at(c);
            new_tr.target = it->second;

            st.transitions[c] = new_tr;
        }
        labels.erase(S_id);
    }
}


void SubsequentialTransducer::sort_label(StateLabel& label) const{
    std::sort(label.begin(), label.end(),
        [](const auto& a, const auto& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second < b.second;
        });

    label.erase(std::unique(label.begin(), label.end()), label.end());
}


std::vector<StateId> SubsequentialTransducer::reverse_topo_sort_states() const{
    std::vector<StateId> topo_sorted;
    std::unordered_map<StateId, unsigned> in_degree;
    std::queue<StateId> q;

    for(const auto& [st_id, st]: this->states){
        in_degree[st_id] = 0;
    }

    for(const auto& [st_id, st]: this->states){
        for(const auto& [c, tr]: st.transitions){
            ++in_degree[tr.target];
        }
    }


    for(const auto& [id, _]: this->states){
        if(in_degree[id] == 0){
            q.push(id);
        }
    }

    while(!q.empty()){
        StateId st_id = q.front();
        q.pop();
        const SubseqState& st = this->states.at(st_id);

        for(auto& [c, tr]: st.transitions){
            --in_degree[tr.target];
            if(in_degree.at(tr.target) == 0){
                q.push(tr.target);
            }
        }
        topo_sorted.push_back(st_id);
    }

    std::reverse(topo_sorted.begin(), topo_sorted.end());
    return topo_sorted;
}


// Съобразявайки, че имаме даг, намираме MSO чрез обратна топо-сортировка
// и MSO на всички наследници
// mso(q) = lcp({lambda(q, a) . mso(p) | delta(q, a) = p})
std::unordered_map<StateId, std::string> SubsequentialTransducer::get_msos() const{
    const std::vector<StateId> rev_topo_sorted = this->reverse_topo_sort_states();
    std::unordered_map<StateId, std::string> msos;

    for(const StateId& st_id: rev_topo_sorted){
        std::string lcp;
        bool lcp_initialized = false;
        const SubseqState& st = this->states.at(st_id);
        if(st.is_final){
            lcp = st.final_output;
            lcp_initialized = true;
        }
        for(auto& [c, tr]: st.transitions){
            if(!lcp_initialized){
                lcp = tr.output + msos.at(tr.target);
                lcp_initialized = true;
            }
            else{
                lcp = this->find_lcp(lcp, tr.output + msos.at(tr.target));
            }
        }
        msos[st_id] = lcp;
    }
    return msos;
}


// Превръщаме в DenseSubsequentialTransducer, за да запазим worst-case линейността на минимизацията
DenseSubsequentialTransducer SubsequentialTransducer::to_dense() const{
    DenseSubsequentialTransducer dense;
    constexpr StateId INVALID = std::numeric_limits<StateId>::max();
    size_t max_id = 0;
    for(const auto& [st_id, st]: this->states){
        if(st_id > max_id) max_id = st_id;
    }

    std::vector<StateId> old_to_new(max_id + 1, INVALID);

    for(const auto& [old_id, old_st]: this->states){
        StateId new_id = dense.states.size();
        old_to_new[old_id] = new_id;

        DenseSubseqState st;
        st.id = new_id;
        st.is_final = old_st.is_final;
        st.final_output = old_st.final_output;

        dense.states.push_back(std::move(st));
    }

    for(const auto& [old_id, old_st]: this->states){
        StateId new_id = old_to_new[old_id];

        for(const auto& [c, tr]: old_st.transitions){
            SubseqTransition new_tr = tr;
            new_tr.target = old_to_new[tr.target];

            dense.states[new_id].transitions.push_back({c, new_tr});
        }
    }

    dense.initial = old_to_new[this->initial];
    dense.initial_output = this->initial_output;
    for(const char& c: this->alphabet){
        dense.alphabet.push_back(c);
    }

    return dense;
}


std::vector<StateId> DenseSubsequentialTransducer::reverse_topo_sort_states() const{
    std::vector<StateId> topo_sorted;
    std::vector<unsigned> in_degree(this->states.size(), 0);
    std::queue<StateId> q;

    for(const auto& st: this->states){
        for(const auto& [c, tr]: st.transitions){
            ++in_degree[tr.target];
        }
    }


    for(const auto& st: this->states){
        if(in_degree[st.id] == 0){
            q.push(st.id);
        }
    }

    while(!q.empty()){
        StateId st_id = q.front();
        q.pop();
        const DenseSubseqState& st = this->states.at(st_id);

        for(auto& [c, tr]: st.transitions){
            --in_degree[tr.target];
            if(in_degree.at(tr.target) == 0){
                q.push(tr.target);
            }
        }
        topo_sorted.push_back(st_id);
    }

    std::reverse(topo_sorted.begin(), topo_sorted.end());
    return topo_sorted;
}


std::vector<std::string> DenseSubsequentialTransducer::get_msos() const{
    const std::vector<StateId> rev_topo_sorted = this->reverse_topo_sort_states();
    std::vector<std::string> msos(this->states.size());

    for(const StateId& st_id: rev_topo_sorted){
        std::string lcp;
        bool lcp_initialized = false;
        const DenseSubseqState& st = this->states.at(st_id);
        if(st.is_final){
            lcp = st.final_output;
            lcp_initialized = true;
        }
        for(auto& [c, tr]: st.transitions){
            if(!lcp_initialized){
                lcp = tr.output + msos.at(tr.target);
                lcp_initialized = true;
            }
            else{
                lcp = this->find_lcp(lcp, tr.output + msos.at(tr.target));
            }
        }
        msos[st_id] = lcp;
    }
    return msos;
}


size_t SubsequentialTransducer::get_transitions_count() const{
    size_t transitions_count = 0;
    for(const auto& [st_id, st]: this->states){
        for(const auto& _: st.transitions){
            ++transitions_count;
        }
    }
    return transitions_count;
}


void DenseSubsequentialTransducer::cannonize(){
    std::vector<std::string> msos = this->get_msos();
    std::string init_out(msos[this->initial].begin(), msos[this->initial].end());
    this->initial_output += init_out;
    for(auto& st: this->states){
        for(auto& [c, tr]: st.transitions){
            std::string mso_st(msos[st.id].begin(), msos[st.id].end());
            std::string mso_target(msos[tr.target].begin(), msos[tr.target].end());
            tr.output = this->remove_prefix(mso_st, tr.output + mso_target);
        }
        if(st.is_final){
            std::string mso_f(msos[st.id].begin(), msos[st.id].end());
            st.final_output = this->remove_prefix(mso_f, st.final_output);
        }
    }
}

std::string DenseSubsequentialTransducer::find_lcp(const std::string& s1, const std::string& s2) const{
    std::string res = "";
    size_t minlen = std::min(s1.size(), s2.size());
    for(size_t i = 0; i < minlen; ++i){
        if(s1[i] == s2[i]) res += s1[i];
        else break;
    }
    return res;
}

std::string DenseSubsequentialTransducer::remove_prefix(const std::string& pref, const std::string& str) const{
    size_t i = pref.size();
    if(pref == "") return str;
    if(str.substr(0,i) != pref) throw std::runtime_error("Something is wrong with the construction.");
    return str.substr(i);
}


size_t SubsequentialTransducer::get_final_states_count() const{
    size_t final_count = 0;
    for(const auto& [st_id, st]: this->states){
        if(st.is_final) ++final_count;
    }
    return final_count;
}
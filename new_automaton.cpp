#include "new_automaton.hpp"
#include "subsequential_transducer.hpp"
#include <algorithm>
#include <iostream>


size_t DeterminedColoredAutomaton::states_count() const{
    return this->states.size();
}


StateId DeterminedColoredAutomaton::create_new_state(){
    DeterminedState new_state;
    new_state.id = this->states_count();
    new_state.is_final = false;
    this->states.push_back(new_state);
    return new_state.id;
}

SymbolId DeterminedColoredAutomaton::get_or_create_symbol(const char& input, const OutputId& out_id){
    unsigned char c = static_cast<unsigned char>(input);

    if(this->symbol_id_by_input_output[c].size() <= out_id){
        this->symbol_id_by_input_output[c].resize(
            out_id + 1,
            std::numeric_limits<SymbolId>::max()
        );
    }

    SymbolId& existing = this->symbol_id_by_input_output[c][out_id];

    if(existing != std::numeric_limits<SymbolId>::max()){
        return existing;
    }

    SymbolId id = alphabet.size();

    alphabet.push_back(Symbol{id, input, out_id});

    existing = id;
    return existing;
}

BlockId DeterminedColoredAutomaton::create_new_block(const unsigned& level){
    Block new_block;
    new_block.level = level;
    BlockId id = this->blocks.size();
    this->blocks.push_back(new_block);
    return id;
}


void DeterminedColoredAutomaton::from_transducer(const SubsequentialTransducer& T){
    const std::unordered_map<StateId, SubseqState>& trans_states = T.get_states();
    
    StateId max_id = 0;
    for(const auto& [st_id, st]: trans_states){
        if(st_id > max_id){
            max_id = st_id;
        }
    }

    constexpr StateId INVALID = std::numeric_limits<StateId>::max();
    
    std::vector<StateId> old_to_new(max_id + 1, INVALID);

    this->symbol_id_by_input_output.resize(256);
    // Минаваме и създаваме всички състояния
    std::cout << "Creating states" << std::endl;
    for(const auto& [st_id, st]: trans_states){
        StateId new_id = this->create_new_state();
        DeterminedState& new_state = this->states.at(new_id);
        new_state.is_final = st.is_final;
        if(st.is_final){
            new_state.final_output_id = intern_output(st.final_output);
        }
        old_to_new.at(st_id) = new_id;
    }

    this->initial = old_to_new.at(T.get_initial());

    std::cout << "Creating transitions" << std::endl;
    // Трупаме новите преходи
    for (const auto& [st_id, st] : trans_states) {
        StateId source = old_to_new.at(st_id);
        auto& out = this->states[source].transitions;

        out.reserve(st.transitions.size());

        for (const auto& [c, tr] : st.transitions) {
            OutputId output_id = this->intern_output(tr.output);
            SymbolId symbol_id = this->get_or_create_symbol(c, output_id);
            StateId target = old_to_new.at(tr.target);

            out.push_back(Arc{symbol_id, target});
        }
    }
}


void DeterminedColoredAutomaton::from_transducer(const DenseSubsequentialTransducer& T){
    const std::vector<DenseSubseqState>& trans_states = T.get_states();
        
    this->symbol_id_by_input_output.resize(256);
    // Минаваме и създаваме всички състояния
    std::cout << "Creating states" << std::endl;
    for(const auto& st: trans_states){
        StateId new_id = this->create_new_state();
        DeterminedState& new_state = this->states.at(new_id);
        new_state.is_final = st.is_final;
        if(st.is_final){
            new_state.final_output_id = intern_output(st.final_output);
        }
    }

    this->initial = T.get_initial();

    std::cout << "Creating transitions" << std::endl;
    for (const auto& st : trans_states) {
        StateId source = st.id;
        auto& out = this->states[source].transitions;

        out.reserve(st.transitions.size());

        for (const auto& [c, tr] : st.transitions) {
            OutputId output_id = this->intern_output(tr.output);
            SymbolId symbol_id = this->get_or_create_symbol(c, output_id);
            StateId target = tr.target;

            out.push_back(Arc{symbol_id, target});
        }
    }
}



void DeterminedColoredAutomaton::construct_L(){
    this->L.clear();
    std::vector<StateId> rev_topo_order = this->reverse_topo();
    std::vector<unsigned> d(this->states.size(), 0);

    for(const StateId& st_id: rev_topo_order){
        DeterminedState& st = this->states[st_id];
        size_t max = 0;
        if(st.transitions.size() == 0){
            d[st_id] = 0;
            if(L.size() < 1) L.resize(1);
            this->L[0].push_back(st_id);
            continue;
        }
        for(auto& tr: st.transitions){
            if(d.at(tr.target) > max) max = d.at(tr.target);
        }
        d[st_id] = max + 1;
        if(L.size() < max + 2) L.resize(max+2);
        this->L[max+1].push_back(st_id);
    }

}


// Обхождаме Trie структурата. Ако има вече такъв запис, му връщаме ID-то,
// ако няма, добавяме го в структурата и връщаме новото ID
OutputId DeterminedColoredAutomaton::intern_output(const std::string& s) {
    if (this->output_trie.empty()) {
        this->output_trie.emplace_back();
    }

    // root node
    int node = 0;

    for (unsigned char c : s) {
        int nxt = this->output_trie[node].next[c];

        // Ако няма такъв наследник, го създаваме
        if (nxt == -1) {
            nxt = this->output_trie.size();
            this->output_trie[node].next[c] = nxt;
            this->output_trie.emplace_back();
        }

        node = nxt;
    }

    // Ако сме стигнали до вече съществуващ терминален node,
    // връщаме неговото ID
    if (this->output_trie[node].terminal) {
       return this->output_trie[node].id;
    }

    // В противен случай, записваме ново ID и връщаме него
    OutputId id = this->outputs_by_id.size();

    this->output_trie[node].terminal = true;
    this->output_trie[node].id = id;
    this->outputs_by_id.push_back(s);

    return id;
}


void DeterminedColoredAutomaton::calculate_classes() {
    // Строим this->incoming
    this->build_incoming();

    // Инициализираме блоковете като за всяко L_i имаме 
    // блок за нефинални състояния, както и блок за 
    // финалните с различни крайни изходи
    this->initialize_level_blocks();

    this->pos_in_block.assign(this->states.size(), 0);

    // Пълним масива, който ни казва кое състояние къде точно в блока си е
    for (BlockId b = 0; b < this->blocks.size(); ++b) {
        for (size_t i = 0; i < this->blocks[b].states.size(); ++i) {
            this->pos_in_block[this->blocks[b].states[i]] = i;
        }
    }

    // Инициализиране на работни масиви

    // При обработката на блок B, за SymbolId sigma,
    // pred_by_symbol[sigma] съдържа всички състояния,
    // които имат преход със sigma до q
    this->pred_by_symbol.resize(alphabet.size());

    // В обработването на текущия блок видели ли сме вече този символ
    this->symbol_seen.assign(alphabet.size(), false);

    // Когато разглеждаме блок В и символ sigma,
    // marked_by_block[C] ни дава състоянията от този блок,
    // които имат sigma-преходи към B
    this->marked_by_block.resize(states.size());

    // В обработването на sigma вече обработили ли сме блок C
    this->block_seen.assign(states.size(), false);

    // Състоянието добавено ли е за текущия символ
    this->state_marked.assign(states.size(), false);

    for(unsigned j = 0; j < L.size() - 1; ++j){
        for(BlockId B: this->level_blocks[j]){
            // Разбиваме предшествениците на B
            this->refine_by_preimages_of_block(B);
        }
    }
    this->assign_class_ids_from_blocks();
}

void DeterminedColoredAutomaton::build_incoming(){
    this->incoming.clear();
    this->incoming.resize(this->states.size());

    for(unsigned i = 0; i < this->states.size(); ++i){
        const DeterminedState& st = this->states[i];
        for(unsigned j = 0; j < st.transitions.size(); ++j){
            InArc new_inarc;
            new_inarc.source = i;
            new_inarc.symbol = st.transitions[j].symbol;
            this->incoming[st.transitions[j].target].push_back(new_inarc);
        }
    }
}

using ColorId = unsigned;

ColorId DeterminedColoredAutomaton::color_of(StateId q) const{
    const DeterminedState& st = this->states[q];

    if(!st.is_final){
        return 0;
    }

    return 1 + st.final_output_id;
}


void DeterminedColoredAutomaton::initialize_level_blocks(){
    this->blocks.clear();
    this->level_blocks.clear();
    this->level_blocks.resize(this->L.size());

    constexpr BlockId INVALID = std::numeric_limits<BlockId>::max();

    this->block_of.assign(this->states.size(), INVALID);

    // По подадено ID на цвят ни казва в кой блок е
    std::vector<BlockId> color_to_block(1 + this->outputs_by_id.size(), INVALID);

    // Пазим кои цветове сме използвали в L_i
    std::vector<ColorId> touched_colors;

    for(unsigned i = 0; i < this->L.size(); ++i){
        touched_colors.clear();

        for(StateId q: this->L[i]){
            // Вземаме ID-то на цвета на q (0, ако не е финално)
            ColorId color = this->color_of(q);

            // Ако този цвят си няма блок го създаваме
            if(color_to_block[color] == INVALID){
                BlockId b = create_new_block(i);
                color_to_block[color] = b;
                touched_colors.push_back(color);
                this->level_blocks[i].push_back(b);
            }

            // Слагаме състоянието q в съответния блок
            BlockId b = color_to_block[color];
            this->blocks[b].states.push_back(q);
            this->block_of[q] = b;
        }
        // Чистим color_to_block, за да може на следващото ниво
        // да няма сливания с блокове от предишното
        for(ColorId color: touched_colors){
            color_to_block[color] = INVALID;
        }
    }
}

// Изфиняване на релацията за конкретен блок
void DeterminedColoredAutomaton::refine_by_preimages_of_block(const BlockId& B){

    // Събираме всички предшественици на B, групирани по символ
    for(StateId q: this->blocks[B].states){
        for(const InArc& in: incoming[q]){
            SymbolId sigma = in.symbol;

            if(!this->symbol_seen[sigma]){
                this->symbol_seen[sigma] = true;
                this->touched_symbols.push_back(sigma);
            }

            this->pred_by_symbol[sigma].push_back(in.source);
        }
    }

    // За всеки символ групиране по текущ блок
    for(SymbolId sigma: this->touched_symbols){
        this->touched_blocks.clear();

        for(StateId p: this->pred_by_symbol[sigma]){

            // Отбелязваме, че p е маркирано и му взимаме блока
            if(this->state_marked[p]) continue;
            state_marked[p] = true;
            BlockId C = this->block_of[p];

            // Отбелязваме, че сме го обработили
            if(!block_seen[C]){
                this->block_seen[C] = true;
                this->touched_blocks.push_back(C);
            }

            // Слагаме p в групата на сечението на (sigma^-1)B с C
            this->marked_by_block[C].push_back(p);
        }
        
        // Разделяне на блоковете
        for(BlockId C: this->touched_blocks){
            std::vector<StateId>& marked = this->marked_by_block[C];

            // Ако (sigma^-1)B е празно или е равно на C не разделяме нищо
            if(marked.empty()) continue;
            if(marked.size() == blocks[C].states.size()) continue;

            // Създаваме нов блок за C пресечено с (sigma^-1)B
            BlockId new_block = this->create_new_block(this->blocks[C].level);
            this->level_blocks[this->blocks[C].level].push_back(new_block);


            for(StateId p: marked){
                std::vector<StateId>& old_states = this->blocks[C].states;

                // Сменяме p с последния елемент на масива,
                // за да го pop-нем за константно време
                size_t pos = this->pos_in_block[p];
                StateId last = old_states.back();

                old_states[pos] = last;
                this->pos_in_block[last] = pos;

                old_states.pop_back();

                // Добавяме p в новия блок
                this->pos_in_block[p] = this->blocks[new_block].states.size();
                this->blocks[new_block].states.push_back(p);
                this->block_of[p] = new_block;
            }
        }

        // Почистваме работните данни за следващата итерация
        for(StateId p: this->pred_by_symbol[sigma]){
            this->state_marked[p] = false;
        }

        for(BlockId C: this->touched_blocks){
            this->marked_by_block[C].clear();
            this->block_seen[C] = false;
        }


        this->pred_by_symbol[sigma].clear();
        this->symbol_seen[sigma] = false;
    }
    touched_symbols.clear();
}


void DeterminedColoredAutomaton::assign_class_ids_from_blocks(){
    constexpr StateId INVALID = std::numeric_limits<StateId>::max();

    this->class_id_of.assign(this->states.size(), INVALID);
    this->representative_of_class.clear();

    StateId next_class = 0;

    // Итерираме по всеки блок и създаваме клас за него
    for(BlockId b = 0; b < this->blocks.size(); ++b){
        // Игнорираме празните блокове
        if(this->blocks[b].states.empty()) continue;

        // Създаваме нов клас с представител първия елемент от блока
        StateId cls = next_class++;
        StateId repr = this->blocks[b].states[0];

        this->representative_of_class.push_back(repr);

        // За всички състояния от блока сетваме тяхното class_id
        for(StateId q: this->blocks[b].states){
            this->class_id_of[q] = cls;
        }
    }
}


const std::string& DeterminedColoredAutomaton::get_final_output(OutputId id) const{
    return this->outputs_by_id.at(id);
}

std::vector<StateId> DeterminedColoredAutomaton::reverse_topo(){
    const size_t n = this->states.size();
    std::vector<StateId> result;
    result.reserve(n);
    std::vector<unsigned> in_degree(n, 0);
    std::queue<StateId> queue;

    for(unsigned st_id = 0; st_id < n; ++st_id){
        DeterminedState st = this->states[st_id];

        for(const Arc& tr: st.transitions){
            ++in_degree[tr.target];
        }
    }

    for(StateId i = 0; i < n; ++i){
        if(in_degree[i] == 0) queue.push(i);
    }

    while(!queue.empty()){
        StateId st_id = queue.front();
        queue.pop();
        result.push_back(st_id);
        
        for(auto& tr: this->states[st_id].transitions){
            --in_degree[tr.target];
            if(in_degree[tr.target] == 0){
                queue.push(tr.target);
            }
        }
    }

    std::reverse(result.begin(), result.end());
    if(result.size() != this->states.size()){
        throw std::runtime_error("Something's wrong with the algorithm");
    }
    return result;
}


DeterminedColoredAutomaton DeterminedColoredAutomaton::get_minimal(){
    // Разбиваме състоянията на множества L_i, където i е 
    // дължината на най-дългата възможна дума от това състояние
    this->construct_L();

    // Намираме класовете на еквивалентност на релацията на Майхил-Нерод
    this->calculate_classes();

    DeterminedColoredAutomaton minimal;

    minimal.alphabet = this->alphabet;
    minimal.outputs_by_id = this->outputs_by_id;

    std::vector<StateId> class_to_new_state(this->representative_of_class.size());

    // За всеки представител на клас създаваме ново състояние
    for(StateId cls = 0; cls < this->representative_of_class.size(); ++cls){
        StateId old_repr = this->representative_of_class[cls];

        StateId new_id = minimal.create_new_state();
        class_to_new_state[cls] = new_id;

        const DeterminedState& old_st = this->states[old_repr];
        DeterminedState& new_st = minimal.states[new_id];

        new_st.is_final = old_st.is_final;
        new_st.final_output_id = old_st.final_output_id;
    }

    minimal.initial = class_to_new_state[this->class_id_of[this->initial]];

    // Пълним преходите, ползвайки представителите на всеки клас от стария автомат
    for(StateId cls = 0; cls < this->representative_of_class.size(); ++cls){
        StateId old_repr = this->representative_of_class[cls];
        StateId new_source = class_to_new_state[cls];

        const DeterminedState& old_st = this->states[old_repr];
        DeterminedState& new_st = minimal.states[new_source];

        for(const Arc& old_arc: old_st.transitions){
            StateId old_target = old_arc.target;
            StateId target_cls = class_id_of[old_target];
            StateId new_target = class_to_new_state[target_cls];

            new_st.transitions.push_back(Arc{old_arc.symbol, new_target});
        }
    }
    return minimal;
}


const std::vector<DeterminedState>& DeterminedColoredAutomaton::get_states() const{
    return this->states;
}

std::pair<std::string, std::string> DeterminedColoredAutomaton::get_symbol(const SymbolId& id) const{
    const Symbol& s = this->alphabet[id];
    return {
        std::string(1, s.input),
        this->outputs_by_id[s.output_id]
    };
}

StateId DeterminedColoredAutomaton::get_initial() const{
    return this->initial;
}
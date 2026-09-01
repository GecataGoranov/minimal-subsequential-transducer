#ifndef _AUTOMATON_HPP
#define _AUTOMATON_HPP

#include <vector>
#include <string>
#include <array>
#include <iostream>


using StateId = unsigned;
class SubsequentialTransducer;
class DenseSubsequentialTransducer;


using SymbolId = unsigned;

struct Arc{
    SymbolId symbol;
    StateId target;

    bool operator==(const Arc& other) const{
        return this->symbol == other.symbol && this->target == other.target;
    }
};


using OutputId = unsigned;
using ColorId = unsigned;

struct DeterminedState{
    StateId id;
    bool is_final;
    OutputId final_output_id = 0;
    std::vector<Arc> transitions;

    bool operator==(const DeterminedState& other) const{
        return this->is_final == other.is_final &&
                this->final_output_id == other.final_output_id &&
                this->transitions == other.transitions;
    }
};

using BlockId = StateId;


struct InArc{
    StateId source;
    SymbolId symbol;
};

struct Block{
    std::vector<StateId> states;
    unsigned level;
};


struct Symbol{
    SymbolId id;
    char input;
    OutputId output_id;
};


struct OutputTrieNode{
    std::array<int, 256> next;
    bool terminal = false;
    OutputId id = 0;

    OutputTrieNode(){
        next.fill(-1);
    }
};

class DeterminedColoredAutomaton{
    private:
        std::vector<DeterminedState> states;
        StateId initial;

        std::vector<Block> blocks; // Всички текущи блокове (множества от еквивалентни според релацията за дадена стъпка състояния)
        std::vector<StateId> block_of; // "мап" от състояние към блок
        std::vector<std::vector<BlockId>> level_blocks; // level_blocks[i] -> кои блокове са от L_i
        std::vector<std::vector<InArc>> incoming; // Всички преходи към q
        std::vector<OutputTrieNode> output_trie; // Масив, представляващ Trie структурата
        std::vector<std::string> outputs_by_id; // По id на финален изход ни връща съответната му дума

        std::vector<Symbol> alphabet;
        std::vector<std::vector<SymbolId>> symbol_id_by_input_output;

        std::vector<std::vector<StateId>> L; // Нивата спрямо дължината на максималната дума

        std::vector<size_t> pos_in_block;

        std::vector<std::vector<StateId>> pred_by_symbol;
        std::vector<char> symbol_seen;
        std::vector<SymbolId> touched_symbols;

        std::vector<std::vector<StateId>> marked_by_block;
        std::vector<char> block_seen;
        std::vector<BlockId> touched_blocks;

        std::vector<char> state_marked;

        std::vector<StateId> class_id_of;
        std::vector<StateId> representative_of_class;

        void from_transducer(const SubsequentialTransducer& T);
        void from_transducer(const DenseSubsequentialTransducer& T);
        size_t states_count() const;
        std::vector<unsigned> d_p();
        size_t bfs(const DeterminedState& st) const;
        void construct_L();
        StateId get_representative(StateId class_id) const;
        void calculate_classes();
        void calc_signature(const StateId q_id);
        std::vector<StateId> reverse_topo();
        
        OutputId intern_output(const std::string& s);
        void build_incoming();
        void initialize_level_blocks();
        void refine_by_preimages_of_block(const BlockId& B);
        void assign_class_ids_from_blocks();

        SymbolId get_or_create_symbol(const char& c, const OutputId& out_id);
        BlockId create_new_block(const unsigned& level);
        ColorId color_of(StateId q) const;

    public:
        DeterminedColoredAutomaton(){};
        DeterminedColoredAutomaton(const SubsequentialTransducer& T){
            this->from_transducer(T);
            std::cout << "Built colored automaton" << std::endl;
        }
        DeterminedColoredAutomaton(const DenseSubsequentialTransducer& T){
            this->from_transducer(T);
            std::cout << "Built colored automaton" << std::endl;
        }
        StateId create_new_state();
        DeterminedColoredAutomaton get_minimal();
        const std::vector<DeterminedState>& get_states() const;
        std::pair<std::string, std::string> get_symbol(const SymbolId& id) const;
        StateId get_initial() const;
        const std::string& get_final_output(OutputId id) const;
};


#endif
#include "subsequential_transducer.hpp"
#include <iostream>

void run_example(const std::string& regex) {

    Transducer T;
    T.from_regex(regex);
    std::cout << "Constructed transducer" << std::endl;

    RealTimeTransducer RT(T);
    std::cout << "Constructed real-time transducer with " << RT.states_count() << " states" << std::endl;
    std::cout << RT.traverse("1325987") << std::endl;

    SubsequentialTransducer S(RT);
    std::cout << "Constructed subsequential transducer" << std::endl;
    std::cout << "Subseq states: " << S.states_count() << std::endl;
    std::cout << "Subseq transitons: " << S.get_transitions_count() << std::endl;


    std::cout << S.traverse("1325987") << std::endl;

    SubsequentialTransducer M = S.minimize();
    std::cout << "Constructed minimal subsequential transducer" << std::endl;
    std::cout << "Minimal states: " << M.states_count() << std::endl;
    std::cout << "Minimal transitions: " << M.get_transitions_count() << std::endl;
    std::cout << "Minimal final states: " << M.get_final_states_count() << std::endl;
    std::cout << M.traverse("1325987") << std::endl;

    std::cout << "--------------------------" << std::endl;
}

int main() {

    std::string zero = "(\"0\"^\"\")";
    std::string zerozero = "(\"00\"^\"\")";
    std::string zerozerozero = "(\"000\"^\"\")";

    std::string from1to9 = "((\"1\"^\"one \") | (\"2\"^\"two \") | (\"3\"^\"three \") | (\"4\"^\"four \") | (\"5\"^\"five \") | (\"6\"^\"six \") | (\"7\"^\"seven \") | (\"8\"^\"eight \") | (\"9\"^\"nine \"))";
    std::string teens = "((\"10\"^\"ten \") | (\"11\"^\"eleven \") | (\"12\"^\"twelve \") | (\"13\"^\"thirteen \") | (\"14\"^\"fourteen \") | (\"15\"^\"fifteen \") | (\"16\"^\"sixteen \") | (\"17\"^\"seventeen \") | (\"18\"^\"eighteen \") | (\"19\"^\"nineteen \"))";
    std::string tens = "((\"2\"^\"twenty \") | (\"3\"^\"thirty \") | (\"4\"^\"forty \") | (\"5\"^\"fifty \") | (\"6\"^\"sixty \") | (\"7\"^\"seventy \") | (\"8\"^\"eighty \") | (\"9\"^\"ninety \"))";

    std::string hundred = "(\"\"^\"hundred \")";
    std::string thousand = "(\"\"^\"thousand \")";
    std::string million = "(\"\"^\"million \")";

    std::string from10to99 = "(" + teens + "| (" + tens + ". (" + zero + "|" + from1to9 + ")))";
    std::string from1to99 = "(" + from1to9 + "|" + teens + "| (" + tens + ". (" + zero + "|" + from1to9 + ")))";
    std::string from00to99 = "(" + zerozero + "| (" + zero + "." + from1to9 + ") |" + from10to99 + ")";
    std::string from100to999 = "(" + from1to9 + "." + hundred + "." + from00to99 + ")";
    std::string from1to999 = "(" + from1to99 + "|" + from100to999 + ")";
    std::string from000to999 = "((" + zero + "." + from00to99 + ") |" + from100to999 + ")";
    std::string from1000to999999 ="(" +  from1to999 + "." + thousand + "." + from000to999 + ")";

    std::string from000000to999999 = "((" + zerozerozero + "." + from000to999 + ") | (((" + zerozero + "." + from1to9 + ") | (" + zero + "." + from10to99 + ") |" + from100to999 + ") ." + thousand + "." + from000to999 + "))";
    
    std::string from1000000to9999999 = "(" + from1to9 + "." + million + "." + from000000to999999 + ")";
    std::string from1to999999 = "(" + from1to999 + "|" + from1000to999999 + ")";
    std::string from1to9999999 = "(" + from1to999 + "|" + from1000to999999 + "|" + from1000000to9999999 + ")";
 
    run_example(from1to9999999);

    return 0;
}

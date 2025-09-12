/**
 * Программа позволяет пользователю точно верифицировать конкретный произвольный вывод.
 * 1. В моменты неоднозначности (когда правило можно применить к нескольким нетерминалам),
 *    программа запрашивает выбор у пользователя, подсвечивая кандидатов.
 * 2. Если вывод успешен, программа выводит:
 *    a) Начальную и итоговую терминальную цепочки.
 *    b) Линейную скобочную форму построенного дерева.
 *    c) Последовательность правил для эквивалентного левого вывода.
 */

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm> 
#include <utility>   

// --- Определение структур ---
struct Rule {
    char lhs;
    std::string rhs;
    Rule(char l, std::string r) : lhs(l), rhs(std::move(r)) {}
};

struct TreeNode {
    std::string value;
    std::vector<TreeNode*> children;
    bool is_terminal;
    TreeNode(std::string val, bool term) : value(std::move(val)), is_terminal(term) {}
    ~TreeNode() { for (auto child : children) { delete child; } }
};

// --- Вспомогательные функции и анализаторы ---
bool parseRuleSequence(const std::string& input, std::vector<int>& output);
int extractRuleNumber(const std::string& value);
void generateBracketForm(const TreeNode* node, std::stringstream& ss);
void findLeftmostSequence(const TreeNode* node, std::vector<int>& sequence);
void getTerminalString(const TreeNode* node, std::stringstream& ss);

// --- Основная логика ---

/** @brief Выводит на экран правила заданной грамматики. */
void printGrammar(const std::vector<Rule>& grammar) {
    std::cout << "--- Грамматика (Вариант 9) ---\n";

    for (size_t i = 1; i < grammar.size(); ++i) {
        std::cout << i << ". " << grammar[i].lhs << " -> " << grammar[i].rhs << "\n";
    }

    std::cout << "-------------------------------\n\n";
}

/**
 * @brief Главная функция, обрабатывающая произвольный вывод в интерактивном режиме.
 */
void processArbitraryDerivation(const std::vector<Rule>& grammar, const std::vector<int>& ruleSequence) {
    std::cout << "\n=========================================================\n";
    std::cout << "Анализ произвольного вывода\n";
    std::cout << "---------------------------------------------------------\n";

    auto* root = new TreeNode("S", false);
    std::vector<TreeNode*> worklist = {root}; // Отслеживает "живые" нетерминалы

    for (size_t i = 0; i < ruleSequence.size(); ++i) {
        int ruleNum = ruleSequence[i];

        if (ruleNum <= 0 || (size_t)ruleNum >= grammar.size()) {
            std::cerr << "[ОШИБКА] Неверный номер правила: " << ruleNum << std::endl;
            delete root; return;
        }

        const Rule& rule = grammar[ruleNum];
        
        std::vector<TreeNode*> candidate_nodes;
        for (TreeNode* node : worklist) {
            if (node->value[0] == rule.lhs) {
                candidate_nodes.push_back(node);
            }
        }

        if (candidate_nodes.empty()) {
            std::stringstream currentString;
            for(auto n : worklist) currentString << n->value;

            std::cerr << "[ОШИБКА] Правило #" << ruleNum << " (" << rule.lhs << " -> ...) неприменимо. Доступные нетерминалы: \"" << currentString.str() << "\"\n";
            
            delete root; 
            return;
        }

        TreeNode* node_to_expand = nullptr;
        if (candidate_nodes.size() == 1) {
            node_to_expand = candidate_nodes[0];
        } else {
            std::cout << "\nШаг " << i + 1 << ": Применяем правило #" << ruleNum << " (" << rule.lhs << " -> " << rule.rhs << ")\n";
            std::cout << "Текущая цепочка нетерминалов: ";
            std::stringstream highlighted_ss;
            int candidate_counter = 1;
            for (TreeNode* node : worklist) {
                bool is_candidate = false;
                for(auto c : candidate_nodes) { if(c == node) { is_candidate = true; break; } }

                if (is_candidate) { 
                    highlighted_ss << "\033[1;31m" << node->value[0] << "_" << candidate_counter++ << "\033[0m ";
                } else {
                    highlighted_ss << node->value << " ";
                }
            }
            std::cout << highlighted_ss.str() << "\n";
            std::cout << "Найдено несколько нетерминалов '" << rule.lhs << "'. Выберите, какой заменить (укажите номер):\n";
            int choice = 0;
            while (true) {
                std::cout << "> ";
                std::cin >> choice;

                if (std::cin.good() && choice > 0 && (size_t)choice <= candidate_nodes.size()) {
                    std::cin.ignore(10000, '\n'); break;
                }

                std::cin.clear(); std::cin.ignore(10000, '\n');
                std::cerr << "Неверный ввод. Пожалуйста, введите число от 1 до " << candidate_nodes.size() << ".\n";
            }

            node_to_expand = candidate_nodes[choice - 1];
        }
        
        node_to_expand->value = std::string(1, rule.lhs) + "<" + std::to_string(ruleNum) + ">";

        for(char symbol : rule.rhs) {
            node_to_expand->children.push_back(new TreeNode(std::string(1, symbol), !isupper(symbol)));
        }

        std::vector<TreeNode*> next_worklist;
        for (TreeNode* node : worklist) {
            if (node == node_to_expand) {
                for (TreeNode* child : node_to_expand->children) {
                    if (!child->is_terminal) { next_worklist.push_back(child); }
                }
            } else { 
                next_worklist.push_back(node); 
            }
        }
        worklist = next_worklist;
    }

    std::cout << "\n---------------------------------------------------------\n";
    std::cout << "Произвольный вывод успешно завершен!\n";
    
    std::stringstream terminal_ss;
    getTerminalString(root, terminal_ss);

    std::cout << "\n-> Начальная цепочка (стартовый символ):\nS\n";
    std::cout << "\n-> Итоговая терминальная цепочка:\n" << terminal_ss.str() << "\n";

    std::stringstream bracket_ss;
    generateBracketForm(root, bracket_ss);

    std::cout << "\n-> Линейная скобочная форма дерева:\n" << bracket_ss.str() << "\n";
    
    std::vector<int> leftmost_sequence;
    findLeftmostSequence(root, leftmost_sequence);

    std::cout << "\n-> Эквивалентная последовательность правил для ЛЕВОГО вывода:\n";

    for(size_t i = 0; i < leftmost_sequence.size(); ++i) {
        std::cout << leftmost_sequence[i] << (i == leftmost_sequence.size() - 1 ? "" : ", ");
    }

    std::cout << "\n=========================================================\n\n";

    delete root;
}

int main() {
    const std::vector<Rule> grammar = {
        Rule{' ', ""}, Rule{'S', "SbSa"}, Rule{'S', "Sa"},   Rule{'S', "A"},
        Rule{'A', "aS"},   Rule{'A', "aB"},   Rule{'A', "b"}, Rule{'B', "b"},
        Rule{'B', "Aa"}
    };

    printGrammar(grammar);
    std::string userInput;

    while (true) {
        std::cout << "Введите последовательность правил `p` (через запятую) или 'exit' для выхода:\n> ";
        std::getline(std::cin, userInput);

        if (userInput == "exit" || userInput == "quit") break;

        if (userInput.empty()) continue;

        std::vector<int> ruleSequence;

        if (parseRuleSequence(userInput, ruleSequence)) {
            processArbitraryDerivation(grammar, ruleSequence);
        }
    }

    std::cout << "\nПрограмма завершена.\n";

    return 0;
}

bool parseRuleSequence(const std::string& input, std::vector<int>& output) {
    output.clear();
    std::stringstream ss(input);
    std::string segment;

    while (std::getline(ss, segment, ',')) {
        try { 
            output.push_back(std::stoi(segment)); 
        } catch (const std::exception&) { 
            std::cerr << "\n[ОШИБКА ВВОДА] Некорректный фрагмент: '" << segment << "'.\n"; return false; 
        }
    }

    return true;
}

int extractRuleNumber(const std::string& value) {
    size_t start = value.find('<');
    size_t end = value.find('>');

    if (start == std::string::npos || end == std::string::npos) 
        return -1;

    return std::stoi(value.substr(start + 1, end - start - 1));
}

void generateBracketForm(const TreeNode* node, std::stringstream& ss) {
    if (!node) return;
    if (node->is_terminal) { ss << node->value; }
    else {
        ss << "[" << node->value;
        for (const auto* child : node->children) { ss << " "; generateBracketForm(child, ss); }
        ss << "]";
    }
}

void findLeftmostSequence(const TreeNode* node, std::vector<int>& sequence) {
    if (!node || node->is_terminal) return;
    int ruleNum = extractRuleNumber(node->value);
    if (ruleNum != -1) { sequence.push_back(ruleNum); }
    for (const auto* child : node->children) { findLeftmostSequence(child, sequence); }
}

void getTerminalString(const TreeNode* node, std::stringstream& ss) {
    if (!node) return;
    if (node->is_terminal) { ss << node->value; return; }
    for (const auto* child : node->children) { getTerminalString(child, ss); }
}
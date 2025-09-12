/**
 * @file arbitrary_derivation_analyzer.cpp
 * @brief Анализирует произвольный вывод, строит дерево и находит эквивалентный левый вывод.
 * @author AI Assistant Gemini
 * @version 5.0
 *
 * Программа выполняет следующие задачи:
 * 1. В интерактивном режиме симулирует произвольный вывод для заданной
 *    пользователем последовательности правил `p`.
 * 2. В случаях неоднозначности (несколько возможных мест для применения правила)
 *    запрашивает выбор у пользователя.
 * 3. В процессе вывода строит структуру дерева.
 * 4. Если вывод успешен, генерирует линейную скобочную форму дерева.
 * 5. Находит и выводит последовательность правил для эквивалентного левого вывода,
 *    который соответствует тому же дереву.
 */

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm> // for std::find

// --- Определение структур ---

/**
 * @struct Rule
 * @brief Хранит одно правило КС-грамматики.
 */
struct Rule {
    char lhs;           ///< Левая часть правила (нетерминал).
    std::string rhs;    ///< Правая часть правила (цепочка символов).
};

/**
 * @struct TreeNode
 * @brief Узел в дереве вывода.
 */
struct TreeNode {
    std::string value;                  ///< Значение узла (например, "S" или "S<1>").
    std::vector<TreeNode*> children;    ///< Дочерние узлы.
    bool is_terminal;                   ///< Флаг, является ли узел терминальным.

    TreeNode(std::string val, bool term) : value(val), is_terminal(term) {}

    /**
     * @brief Деструктор для рекурсивного освобождения памяти.
     */
    ~TreeNode() {
        for (auto child : children) {
            delete child;
        }
    }
};


// --- Вспомогательные функции ---

/** @brief Разбирает строку с правилами, разделенными запятыми, в вектор чисел. */
bool parseRuleSequence(const std::string& input, std::vector<int>& output) {
    output.clear();
    std::stringstream ss(input);
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        try {
            output.push_back(std::stoi(segment));
        } catch (const std::exception&) {
            std::cerr << "\n[ОШИБКА ВВОДА] Некорректный фрагмент: '" << segment << "'.\n";
            return false;
        }
    }
    return true;
}

/** @brief Извлекает номер правила из строки вида "S<12>". */
int extractRuleNumber(const std::string& value) {
    size_t start = value.find('<');
    size_t end = value.find('>');
    if (start == std::string::npos || end == std::string::npos) return -1;
    return std::stoi(value.substr(start + 1, end - start - 1));
}


// --- Функции-анализаторы дерева ---

/** @brief Рекурсивно генерирует линейную скобочную форму из дерева. */
void generateBracketForm(const TreeNode* node, std::stringstream& ss) {
    if (!node) return;
    if (node->is_terminal) {
        ss << node->value;
    } else {
        ss << "[" << node->value;
        for (const auto* child : node->children) {
            ss << " ";
            generateBracketForm(child, ss);
        }
        ss << "]";
    }
}

/**
 * @brief Находит эквивалентную левую последовательность правил путем префиксного обхода дерева.
 * @param node Текущий узел дерева.
 * @param sequence Вектор для сохранения найденной последовательности.
 */
void findLeftmostSequence(const TreeNode* node, std::vector<int>& sequence) {
    if (!node || node->is_terminal) {
        return;
    }
    // Pre-order traversal: Visit root, then traverse children.
    int ruleNum = extractRuleNumber(node->value);
    if (ruleNum != -1) {
        sequence.push_back(ruleNum);
    }
    for (const auto* child : node->children) {
        findLeftmostSequence(child, sequence);
    }
}


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
 * @brief Главная функция, обрабатывающая произвольный вывод.
 *
 * Симулирует произвольный вывод, запрашивая у пользователя выбор в случае
 * неоднозначности. Одновременно строит дерево вывода. Если вывод успешен,
 * запускает анализ построенного дерева.
 */
void processArbitraryDerivation(const std::vector<Rule>& grammar, const std::vector<int>& ruleSequence) {
    std::cout << "\n=========================================================\n";
    std::cout << "Анализ произвольного вывода\n";
    std::cout << "---------------------------------------------------------\n";

    // --- Этап 1: Интерактивный вывод и построение дерева ---
    auto* root = new TreeNode("S", false);
    std::string currentString = "S";
    // `worklist` отслеживает нетерминальные узлы дерева, соответствующие нетерминалам в строке
    std::vector<TreeNode*> worklist = {root};

    for (size_t i = 0; i < ruleSequence.size(); ++i) {
        int ruleNum = ruleSequence[i];
        if (ruleNum <= 0 || (size_t)ruleNum >= grammar.size()) {
            std::cerr << "[ОШИБКА] Неверный номер правила: " << ruleNum << std::endl;
            delete root; return;
        }
        const Rule& rule = grammar[ruleNum];
        
        // Находим все возможные места для применения правила
        std::vector<int> candidate_indices;
        std::vector<TreeNode*> candidate_nodes;
        size_t current_pos = 0;
        for (size_t j = 0; j < worklist.size(); ++j) {
            if (worklist[j]->value[0] == rule.lhs) {
                candidate_indices.push_back(current_pos);
                candidate_nodes.push_back(worklist[j]);
            }
            current_pos += worklist[j]->value.length();
        }

        if (candidate_nodes.empty()) {
            std::cerr << "[ОШИБКА] Правило #" << ruleNum << " (" << rule.lhs << " -> ...) неприменимо к нетерминалам в строке: \"" << currentString << "\"\n";
            delete root; return;
        }

        TreeNode* node_to_expand = nullptr;
        int string_pos_to_expand = -1;
        
        if (candidate_nodes.size() == 1) {
            node_to_expand = candidate_nodes[0];
        } else {
            // Запрашиваем выбор пользователя
            std::cout << "\nШаг " << i + 1 << ": Применяем правило #" << ruleNum << " (" << rule.lhs << " -> " << rule.rhs << ")\n";
            std::cout << "Текущая цепочка: \"" << currentString << "\"\n";
            std::cout << "Найдено несколько нетерминалов '" << rule.lhs << "'. Выберите, какой заменить (укажите номер):\n";
            for(size_t k = 0; k < candidate_nodes.size(); ++k) {
                std::cout << "  " << k + 1 << ". Нетерминал в позиции " << candidate_indices[k] << "\n";
            }
            int choice = 0;
            while (true) {
                std::cout << "> ";
                std::cin >> choice;
                if (std::cin.good() && choice > 0 && (size_t)choice <= candidate_nodes.size()) {
                    std::cin.ignore(10000, '\n'); // Очистка буфера ввода
                    break;
                }
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cerr << "Неверный ввод. Пожалуйста, введите число от 1 до " << candidate_nodes.size() << ".\n";
            }
            node_to_expand = candidate_nodes[choice - 1];
        }
        
        // Применяем правило: обновляем узел дерева и рабочий список
        node_to_expand->value = std::string(1, rule.lhs) + "<" + std::to_string(ruleNum) + ">";
        std::vector<TreeNode*> new_children;
        for(char symbol : rule.rhs) {
            new_children.push_back(new TreeNode(std::string(1, symbol), !isupper(symbol)));
        }
        node_to_expand->children = new_children;

        // Обновляем worklist: заменяем родителя на его дочерние нетерминалы
        auto it = std::find(worklist.begin(), worklist.end(), node_to_expand);
        it = worklist.erase(it);
        for(auto child : new_children) {
            if(!child->is_terminal) {
                it = worklist.insert(it, child);
                it++;
            }
        }
        
        // Обновляем строку для отображения
        std::stringstream ss_str;
        for(auto node : worklist) { ss_str << node->value; }
        currentString = ss_str.str();
    }

    // --- Этап 2: Анализ и вывод результатов ---
    std::cout << "\n---------------------------------------------------------\n";
    std::cout << "Произвольный вывод успешно завершен!\n";
    
    std::stringstream final_ss;
    std::vector<int> leftmost_sequence;
    
    generateBracketForm(root, final_ss);
    findLeftmostSequence(root, leftmost_sequence);

    std::cout << "\n-> Линейная скобочная форма дерева:\n" << final_ss.str() << "\n";
    
    std::cout << "\n-> Эквивалентная последовательность правил для ЛЕВОГО вывода:\n";
    for(size_t i = 0; i < leftmost_sequence.size(); ++i) {
        std::cout << leftmost_sequence[i] << (i == leftmost_sequence.size() - 1 ? "" : ", ");
    }
    std::cout << "\n=========================================================\n\n";

    delete root;
}


int main() {
    const std::vector<Rule> grammar = {
        {},
        {'S', "SbSa"}, {'S', "Sa"},   {'S', "A"},
        {'A', "aS"},   {'A', "aB"},   {'A', "b"},
        {'B', "b"},    {'B', "Aa"}
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
#include <iostream>
#include <vector>
#include <string>
#include <memory>  
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <utility>  

/**
 * @struct Rule
 * @brief Хранит одно правило грамматики.
 */
struct Rule {
    char non_terminal;
    std::string replacement;
};

/**
 * @class TreeNode
 * @brief Узел в дереве вывода.
 */
class TreeNode {
public:
    std::string value;
    int rule_number = 0; // Номер правила, примененного для создания этого узла
    std::vector<std::unique_ptr<TreeNode>> children;

    explicit TreeNode(std::string val) : value(std::move(val)) {}

    // Запрещаем копирование, чтобы избежать проблем с владением памятью
    TreeNode(const TreeNode&) = delete;
    TreeNode& operator=(const TreeNode&) = delete;
};

/**
 * @class DerivationTree
 * @brief Представляет и строит дерево вывода (реализует логику вашего класса Tree).
 */
class DerivationTree {
public:
    DerivationTree(char start_symbol) {
        root = std::make_unique<TreeNode>(std::string(1, start_symbol));
    }

    /**
     * @brief Находит самый левый узел для замены и добавляет к нему дочерние элементы.
     */
    void apply_rule(const Rule& rule, int rule_num) {
        TreeNode* node_to_expand = find_leftmost_expandable_node(root.get(), rule.non_terminal);
        if (!node_to_expand) {
            throw std::runtime_error("Ошибка дерева: Не найден подходящий нетерминал для применения правила.");
        }
        
        node_to_expand->rule_number = rule_num;
        for (char symbol : rule.replacement) {
            node_to_expand->children.push_back(std::make_unique<TreeNode>(std::string(1, symbol)));
        }
    }

    /**
     * @brief Генерирует линейную скобочную форму дерева (реализует логику getTree).
     */
    std::string get_bracket_form() const {
        std::stringstream ss;
        generate_bracket_form_recursive(root.get(), ss);
        return ss.str();
    }

private:
    std::unique_ptr<TreeNode> root;

    // Рекурсивный поиск самого левого нераскрытого нетерминала (аналог findNode)
    TreeNode* find_leftmost_expandable_node(TreeNode* current, char target_non_terminal) {
        if (!current) return nullptr;
        // Если узел - искомый нетерминал и у него еще нет дочерних узлов, мы нашли его.
        if (current->value.length() == 1 && current->value[0] == target_non_terminal && current->children.empty()) {
            return current;
        }
        // В противном случае, рекурсивно ищем в дочерних узлах слева направо.
        for (const auto& child : current->children) {
            TreeNode* result = find_leftmost_expandable_node(child.get(), target_non_terminal);
            if (result) {
                return result;
            }
        }
        return nullptr;
    }

    // Рекурсивная генерация скобочной формы
    void generate_bracket_form_recursive(const TreeNode* node, std::stringstream& ss) const {
        if (!node) return;

        ss << node->value[0];
        if (isupper(node->value[0])) { // Если это нетерминал
            ss << node->rule_number;
            if (!node->children.empty()) {
                ss << "(";
                for (const auto& child : node->children) {
                    generate_bracket_form_recursive(child.get(), ss);
                }
                ss << ")";
            }
        }
    }
};

/**
 * @class DerivationProcessor
 * @brief Управляет процессом левого вывода (реализует логику move).
 */
class DerivationProcessor {
public:
    explicit DerivationProcessor(std::string start_chain) : current_chain(std::move(start_chain)) {}

    /**
     * @brief Применяет правило к текущей цепочке, соблюдая левый вывод.
     */
    void apply_rule(const Rule& rule) {
        for (size_t i = 0; i < current_chain.length(); ++i) {
            if (current_chain[i] == rule.non_terminal) {
                current_chain.replace(i, 1, rule.replacement);
                return;
            }
        }
        throw std::runtime_error("Ошибка вывода: Не найден подходящий нетерминал '" + std::string(1, rule.non_terminal) + "' для применения правила.");
    }
    
    const std::string& get_chain() const { return current_chain; }

private:
    std::string current_chain;
};

// --- Основная функция ---

void print_rules(const std::vector<Rule>& rules) {
    std::cout << "--- Грамматика (Вариант 9) ---\n";
    for(size_t i = 0; i < rules.size(); ++i) {
        std::cout << i + 1 << ". " << rules[i].non_terminal << " -> " << rules[i].replacement << "\n";
    }
    std::cout << "------------------------------\n\n";
}

int main() {
    // Используем вашу грамматику "Вариант 9"
    const std::vector<Rule> rules = {
        {'S', "SbSa"}, {'S', "Sa"},   {'S', "A"},
        {'A', "aS"},   {'A', "aB"},   {'A', "b"},
        {'B', "b"},    {'B', "Aa"}
    };
    
    print_rules(rules);

    std::cout << "Введите последовательность правил (например, 357):\n> ";
    std::string rule_sequence_str;
    std::cin >> rule_sequence_str;

    try {
        DerivationProcessor processor("S");
        DerivationTree tree('S');

        std::cout << "\n-> Процесс левого вывода:\n\n";
        std::string display_chain = "S";
        std::cout << "   " << display_chain;

        for (char rule_char : rule_sequence_str) {
            if (!isdigit(rule_char)) {
                throw std::runtime_error("Ошибка ввода: '" + std::string(1, rule_char) + "' не является цифрой.");
            }
            int rule_index = rule_char - '1'; // '1' -> 0, '2' -> 1, ...
            if (rule_index < 0 || (size_t)rule_index >= rules.size()) {
                throw std::runtime_error("Ошибка ввода: Правила с номером " + std::string(1, rule_char) + " не существует.");
            }
            const Rule& selected_rule = rules[rule_index];

            // Модифицируем отображаемую строку ПЕРЕД заменой
            bool rule_added = false;
            for(size_t i = 0; i < display_chain.length(); ++i) {
                if(isupper(display_chain[i])) {
                    display_chain.insert(i + 1, std::to_string(rule_index + 1));
                    rule_added = true;
                    break;
                }
            }
            std::cout << " => " << display_chain;

            // Применяем правило к дереву и реальной цепочке
            tree.apply_rule(selected_rule, rule_index + 1);
            processor.apply_rule(selected_rule);
            display_chain = processor.get_chain();
        }
        std::cout << " => " << processor.get_chain() << "\n\n";

        // Проверка результата
        const std::string final_chain = processor.get_chain();
        for (char c : final_chain) {
            if (isupper(c)) {
                std::cout << "[ПРЕДУПРЕЖДЕНИЕ] Итоговая цепочка не является терминальной. Найден нетерминал '" << c << "'.\n";
                break;
            }
        }

        std::cout << "-> Итоговая цепочка:\n   " << final_chain << "\n\n";
        std::cout << "-> Линейная скобочная форма дерева вывода:\n   " << tree.get_bracket_form() << "\n\n";

    } catch (const std::runtime_error& e) {
        std::cerr << "\n[ПРОИЗОШЛА ОШИБКА]\n" << e.what() << "\n\n";
        return 1;
    }

    return 0;
}
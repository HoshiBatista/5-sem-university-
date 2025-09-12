/**
 * Программа выполняет следующие задачи:
 * 1. Загружает предопределенную КС-грамматику (Вариант 9).
 * 2. В интерактивном режиме запрашивает у пользователя последовательность номеров правил.
 * 3. Выполняет симуляцию левого вывода, сохраняя состояние цепочки на каждом шаге.
 * 4. Выводит пошаговый процесс левого вывода.
 * 5. Формирует и выводит линейную скобочную форму дерева.
 * 6. Строит и визуализирует псевдографическое дерево вывода. Для каждого узла,
 *    где применялось правило, отображается полное состояние выводимой цепочки
 *    сразу после применения этого правила.
 */

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>

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
    std::string value;                  ///< Значение узла (например, "S<1>" или терминал "a").
    std::string derivation_state_string;///< Полная рабочая цепочка после применения правила в этом узле.
    std::vector<TreeNode*> children;    ///< Дочерние узлы.

    /**
     * @brief Деструктор для рекурсивного освобождения памяти, выделенной под дерево.
     */
    ~TreeNode() {
        for (auto child : children) {
            delete child;
        }
    }
};


// --- Вспомогательные функции ---

/**
 * @brief Проверяет, является ли символ нетерминалом (прописная буква).
 * @param c Символ для проверки.
 * @return true, если символ - нетерминал.
 */
bool isNonTerminal(char c) {
    return isupper(c);
}

/**
 * @brief Находит индекс самого левого нетерминала в строке.
 * @param s Строка для поиска.
 * @return Индекс первого найденного нетерминала или -1, если не найден.
 */
int findLeftmostNonTerminalIndex(const std::string& s) {
    for (size_t i = 0; i < s.length(); ++i) {
        if (isNonTerminal(s[i])) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Разбирает строку с правилами, разделенными запятыми, в вектор чисел.
 * @param input Входная строка (например, "1, 2, 3").
 * @param output Выходной вектор для сохранения чисел.
 * @return true, если разбор успешен, иначе false.
 */
bool parseRuleSequence(const std::string& input, std::vector<int>& output) {
    output.clear();
    std::stringstream ss(input);
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        try {
            output.push_back(std::stoi(segment));
        } catch (const std::invalid_argument&) {
            std::cerr << "\n[ОШИБКА ВВОДА] Фрагмент '" << segment << "' не является корректным числом.\n";
            return false;
        } catch (const std::out_of_range&) {
            std::cerr << "\n[ОШИБКА ВВОДА] Число '" << segment << "' слишком большое.\n";
            return false;
        }
    }

    return true;
}


// --- Функции построения и отрисовки дерева ---

/**
 * @brief Рекурсивно строит дерево вывода, используя предварительно вычисленные состояния вывода.
 * @param nonTerminal Текущий нетерминал, для которого строится поддерево.
 * @param grammar Ссылка на вектор правил грамматики.
 * @param ruleSequence Последовательность применяемых правил.
 * @param currentRuleIndex Ссылка на индекс текущего правила в последовательности.
 * @param derivationStates Вектор, содержащий состояния всей цепочки на каждом шаге вывода.
 * @return Указатель на корень построенного поддерева.
 */
TreeNode* buildTree(char nonTerminal, const std::vector<Rule>& grammar, const std::vector<int>& ruleSequence, size_t& currentRuleIndex, const std::vector<std::string>& derivationStates) {
    auto* node = new TreeNode();
    if (currentRuleIndex >= ruleSequence.size()) {
        node->value = "[ОШИБКА: Последовательность правил слишком коротка]";
        return node;
    }
    int ruleNum = ruleSequence[currentRuleIndex];
    
    // Проверяем корректность правила
    if (ruleNum <= 0 || (size_t)ruleNum >= grammar.size() || grammar[ruleNum].lhs != nonTerminal) {
         node->value = "[ОШИБКА: Некорректное применение правила #" + std::to_string(ruleNum) + "]";
         return node;
    }
    
    // Присваиваем значения узлу
    node->value = std::string(1, nonTerminal) + "<" + std::to_string(ruleNum) + ">";
    // Состояние цепочки ПОСЛЕ применения правила с индексом currentRuleIndex
    node->derivation_state_string = derivationStates[currentRuleIndex + 1];
    
    currentRuleIndex++;
    const Rule& rule = grammar[ruleNum];
    
    for (char symbol : rule.rhs) {
        if (isNonTerminal(symbol)) {
            node->children.push_back(buildTree(symbol, grammar, ruleSequence, currentRuleIndex, derivationStates));
        } else {
            auto* leaf = new TreeNode();
            leaf->value = std::string(1, symbol);
            node->children.push_back(leaf);
        }
    }
    return node;
}

/**
 * @brief Рекурсивно обходит дерево и генерирует его линейную скобочную форму.
 * @param node Текущий узел дерева.
 * @param ss Поток для записи результата.
 */
void generateBracketForm(const TreeNode* node, std::stringstream& ss) {
    if (!node) return;

    if (node->children.empty()) {
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
 * @brief Вспомогательная рекурсивная функция для отрисовки псевдографического дерева.
 * @param node Узел дерева для отрисовки.
 * @param prefix Строка-префикс для форматирования.
 * @param isLast Является ли этот узел последним в списке дочерних узлов.
 */
void printAsciiTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLast) {
    if (!node) return;
    std::cout << prefix << (isLast ? "└─ " : "├─ ") << node->value;
    if (!node->derivation_state_string.empty()) {
        std::cout << " -> \"" << node->derivation_state_string << "\"";
    }
    std::cout << std::endl;
    
    std::string childPrefix = prefix + (isLast ? "    " : "│   ");
    for (size_t i = 0; i < node->children.size(); ++i) {
        printAsciiTreeRecursive(node->children[i], childPrefix, i == node->children.size() - 1);
    }
}

/**
 * @brief Запускает отрисовку дерева вывода в псевдографическом виде.
 * @param root Указатель на корневой узел дерева.
 */
void printAsciiTree(const TreeNode* root) {
    if (!root) return;
    std::cout << root->value;
    if (!root->derivation_state_string.empty()) {
        std::cout << " -> \"" << root->derivation_state_string << "\"";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < root->children.size(); ++i) {
        printAsciiTreeRecursive(root->children[i], "", i == root->children.size() - 1);
    }
}


// --- Основная логика ---

/**
 * @brief Выводит на экран правила заданной грамматики.
 * @param grammar Вектор правил.
 */
void printGrammar(const std::vector<Rule>& grammar) {
    std::cout << "--- Грамматика (Вариант 9) ---\n";
    for (size_t i = 1; i < grammar.size(); ++i) {
        std::cout << i << ". " << grammar[i].lhs << " -> " << grammar[i].rhs << "\n";
    }
    std::cout << "-------------------------------\n\n";
}

/**
 * @brief Обрабатывает одну последовательность правил: строит вывод и оба представления дерева.
 * @param title Заголовок для блока вывода.
 * @param grammar Ссылка на грамматику.
 * @param ruleSequence Последовательность правил для обработки.
 */
void processDerivation(const std::string& title, const std::vector<Rule>& grammar, const std::vector<int>& ruleSequence) {
    std::cout << "\n=========================================================\n";
    std::cout << title << "\n";
    std::cout << "---------------------------------------------------------\n";

    // --- Этап 1: Симуляция левого вывода и сбор состояний ---
    std::vector<std::string> derivationStates;
    std::string currentString = "S";
    derivationStates.push_back(currentString);

    std::cout << "-> Левый вывод:\n\n   " << currentString << "\n";
    for (int ruleNum : ruleSequence) {
        int index = findLeftmostNonTerminalIndex(currentString);
        if (index == -1) {
            std::cout << "\n[ОШИБКА ВЫВОДА] В цепочке нет нетерминалов, но правила еще есть.\n";
            return;
        }
        char leftmostNonTerminal = currentString[index];
        if (ruleNum <= 0 || (size_t)ruleNum >= grammar.size() || grammar[ruleNum].lhs != leftmostNonTerminal) {
             std::cout << "\n[ОШИБКА ВЫВОДА] Правило #" << ruleNum << " неприменимо к '" << leftmostNonTerminal << "'.\n";
             return;
        }
        
        const Rule& ruleToApply = grammar[ruleNum];
        currentString.replace(index, 1, ruleToApply.rhs);
        derivationStates.push_back(currentString); // Сохраняем состояние ПОСЛЕ применения
        std::cout << "   => " << currentString << "\n";
    }
    std::cout << "\n-> Итоговая терминальная цепочка: " << currentString << "\n";

    // --- Этап 2: Построение дерева и генерация представлений ---
    size_t ruleIndex = 0;
    TreeNode* root = buildTree('S', grammar, ruleSequence, ruleIndex, derivationStates);

    std::stringstream bracket_ss;
    generateBracketForm(root, bracket_ss);
    std::cout << "\n-> Линейная скобочная форма:\n\n" << bracket_ss.str() << "\n";

    std::cout << "\n-> Дерево вывода (с состояниями цепочки):\n\n";
    printAsciiTree(root);
    
    delete root; // Освобождаем память
    std::cout << "\n=========================================================\n\n";
}


/**
 * @brief Главная функция программы.
 * @return 0 в случае успешного завершения.
 */
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
        std::cout << "Введите последовательность правил (через запятую) или 'exit' для выхода:\n> ";
        std::getline(std::cin, userInput);

        if (userInput == "exit" || userInput == "quit") break;
        if (userInput.empty()) continue;

        std::vector<int> ruleSequence;
        if (parseRuleSequence(userInput, ruleSequence)) {
            if (ruleSequence.empty()) {
                 std::cout << "[ПРЕДУПРЕЖДЕНИЕ] Вы ввели пустую последовательность.\n\n";
                 continue;
            }
            processDerivation("Результат для введенной последовательности", grammar, ruleSequence);
        } else {
            std::cout << "Пожалуйста, попробуйте еще раз.\n\n";
        }
    }

    std::cout << "\nПрограмма завершена.\n";

    return 0;
}
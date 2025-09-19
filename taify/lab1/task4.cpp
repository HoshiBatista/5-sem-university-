#include <iostream>
#include <vector>
#include <string>

typedef struct node{
    node **el;
    int countEl;
    char *characters;
    int size;
    char *rules;
}node;

class Tree {
    node *root;
    void findNode(node *n, bool &end, node *&ans, char c, char p);
    Tree() = default;
public:
    Tree(char c);
    void setNode(char *array, char c, char p);
    void getTree(node *n, std::vector<char> &str, int &index);
    ~Tree();
    node * getRoot();
};

Tree::Tree(char c){
    root = new node();
    root->characters = new char[2];
    root->characters[0] = c;
    root->characters[1] = '\0';
    root->size = 1;
    root->countEl = 0;
    root->el = nullptr;
    root->rules = new char[root->size];
}

void Tree::findNode(node *n, bool &end, node *&ans, char c, char p){
    if(end){
        return;
    }
    if(n->countEl == 0){
        for(int i = 0; i < n->size; i++){
            if(n->characters[i] < 97 && n->characters[i] == c){
                end = true;
                n->el = new node*[n->size];
                n->countEl = n->size;
                for(int j = 0; j < n->size; j++){
                    if(i!=j){
                        n->el[j] = nullptr;
                    } else{
                        n->el[j] = new node();
                        ans = n->el[j];
                        n->rules[j] = p;
                    }
                }
                return;
            }
        }
    }
    else{
        for(int i = 0; i < n->size; i++){
            if(n->characters[i] < 97 && n->el[i] == nullptr && !end && n->characters[i] == c){
                end = true;
                n->el[i] = new node();
                ans = n->el[i];
                n->rules[i] = p;
                return;
            }
            else if(n->el[i] != nullptr){
                if(!end) {
                    findNode(n->el[i], end, ans, c, p);
                }
            }
        }
    }
}

void Tree::setNode(char *array, char x, char p){
    char *c = array;
    int count = 0;
    while (*c!= '\0'){
        count++;
        c++;
    }
    node *curr = nullptr;
    bool end = false;
    findNode(root, end, curr, x, p);
    if(curr) {
        curr->characters = new char[count + 1];
        for(int i = 0 ; i < count; i++){
            curr->characters[i] = array[i];
        }
        curr->characters[count] = '\0';
        curr->size = count;
        curr->countEl = 0;
        curr->el = nullptr;
        curr->rules = new char[count];
    }
}

void Tree::getTree(node *n, std::vector<char> &str, int &index){
    for(int i = 0; i < n->size; i++){
        str.push_back(n->characters[i]);
        if(n->characters[i] < 97){
            str.push_back(n->rules[i]);
            str.push_back('(');
        }
        if(n->el != nullptr && n->el[i] != nullptr){
            getTree(n->el[i], str, index);
            str.push_back(')');
        }
    }
}

void deleteTree(node *n){
    if (n == nullptr) return;
    for(int i = 0; i < n->size; i++){
        if(n->el != nullptr && n->el[i] != nullptr){
            deleteTree(n->el[i]);
            delete n->el[i];
        }
    }
    delete[] n->el;
    delete[] n->characters;
    delete[] n->rules;
}

Tree::~Tree(){
    deleteTree(root);
}

node * Tree::getRoot(){
    return root;
}

typedef struct P{
    std::string x;
    std::string y;
}P;

int move(P p, char *&str, int &capacity, int &size){
    char current = '\0';
    for(int i = 0; i < size; i++){
        if(str[i] < 96 && str[i] == p.x[0]){
            current = str[i];
            break;
        }
    }
    if(current == '\0'){
        std::cout << "\nНетерминалы не найдены";
        return 1;
    }

    int new_len = size + p.y.length() - p.x.length();
    char *tempStr = new char[new_len + 1];
    bool change = false;
    int newSize = 0;
    for(int i = 0; i < size; i++){
        if(!change && str[i] == current){
            for(char c : p.y){
                tempStr[newSize++] = c;
            }
            change = true;
        }
        else{
            tempStr[newSize++] = str[i];
        }
    }

    if(new_len > capacity){
        delete[] str;
        capacity = new_len * 2;
        str = new char[capacity+1];
    }

    for(int i = 0; i < newSize; i++){
        str[i] = tempStr[i];
    }
    size = newSize;
    delete[] tempStr;
    return 0;
}

char *setRule(char *c, char rule, int &size, int &capacity, char x){
    char *res = new char[capacity];
    char *begin = c;
    int flag = 1;
    int sizeRes = 0;
    while(*begin != '\0'){
        res[sizeRes++] = *begin;
        if(*begin < 97 && flag && x == *begin){
            flag = 0;
            res[sizeRes++] = rule;
        }
        begin++;
    }
    res[sizeRes] = '\0';
    return res;
}

char *solve(Tree &tree, char buf[], const std::vector<char> *v = nullptr){

    std::string sequence_str = "";
    if (v != nullptr) {
        for (char ch : *v) {
            sequence_str += ch;
        }
        // Копируем в buf, так как остальная часть кода его использует
        for(size_t i = 0; i < sequence_str.length(); ++i) {
            buf[i] = sequence_str[i];
        }
        buf[sequence_str.length()] = '\0';
    } else {
        sequence_str = buf;
    }


    P p1 = {"S", "SbSa"};
    P p2 = {"S", "Sa"};
    P p3 = {"S", "A"};
    P p4 = {"A", "aS"};
    P p5 = {"A", "aB"};
    P p6 = {"A", "b"};
    P p7 = {"B", "b"};
    P p8 = {"B", "Aa"};
    P rules[] = {p1, p2, p3, p4, p5, p6, p7, p8};

    char *ans = new char[21];
    ans[0] = 'S';
    int size = 1;
    int capacity = 20;
    
    std::cout << "S"; 

    for(char c : sequence_str){
        std::cout << " => ";
        
        if(c > '8' || c < '1'){
            std::cout << "\nНеверное обозначение: " << c << std::endl;
            exit(1);
        }

        int rule_idx = c - '1';
        
        ans[size] = '\0';
        char *result = setRule(ans, c, size, capacity, rules[rule_idx].x[0]);
        std::cout << result;
        delete[] result;

        if (v == nullptr) { 
             tree.setNode(&rules[rule_idx].y[0], rules[rule_idx].x[0], c);
        }
       
        if(move(rules[rule_idx], ans, capacity, size) == 1){
            exit(1);
        }
    }

    ans[size] = '\0';
    std::cout << " => " << ans;

    return ans;
}

int main() {
    std::cout << "Введите последовательность правил\n> ";
    char buf[100];
    std::cin >> buf;

    std::cout << "\n=========================================================\n";
    Tree tree = Tree('S');
    char *ans = solve(tree, buf);
    std::cout << "\n\n";

    char *begin = ans;
    while (*begin != '\0'){
        if(*begin < 97){
            std::cout << "[ОШИБКА] Последовательность правил не привела к терминальной цепочке, существует нетерминал: " << *begin << std::endl;
            delete[] ans;
            return 1;
        }
        begin++;
    }

    std::cout << "=========================================================\n";

    std::vector<char> res;
    int index = 0;
    tree.getTree(tree.getRoot(), res, index);
    
    std::cout << "-> Линейная скобочная форма дерева вывода:\n   ";
    for(char c : res){
        std::cout << c;
    }
    std::cout << "\n\n";

    std::vector<char> ruleV;

    std::cout << "\n-> Последовательность правил для левого вывода:\n";

    for(char c : res){ 
        if(c <= '9' && c >= '1') {
            std::cout << c;
            ruleV.push_back(c);
        }
    }
  

    std::cout << "\n\n-> Левый вывод: \n";
    Tree dummy_tree('S');
    char buf2[100]; 
    char* ans2 = solve(dummy_tree, buf2, &ruleV);
    
    std::cout << "\n=========================================================\n\n";

    delete[] ans;
    delete[] ans2;
    return 0;
}
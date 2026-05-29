#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>

using namespace std;

struct Denomination {
    int value;
    int available;
};

vector<Denomination> wallet;
int amount;
string strategy;
vector<pair<int, int>> bestDispense;
bool found = false;

// Функция для удаления пробелов
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

// Парсинг JSON файла
bool parseJSON(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return false;
    }
    
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    
    // Поиск wallet
    size_t walletPos = content.find("\"wallet\"");
    if (walletPos == string::npos) return false;
    
    size_t arrayStart = content.find('[', walletPos);
    if (arrayStart == string::npos) return false;
    
    // Парсинг wallet
    size_t pos = arrayStart;
    while (true) {
        pos = content.find('[', pos + 1);
        if (pos == string::npos) break;
        
        size_t end = content.find(']', pos);
        if (end == string::npos) break;
        
        string item = content.substr(pos + 1, end - pos - 1);
        
        // Парсим два числа
        size_t comma = item.find(',');
        if (comma == string::npos) continue;
        
        string valStr = trim(item.substr(0, comma));
        string availStr = trim(item.substr(comma + 1));
        
        int value = stoi(valStr);
        int available = stoi(availStr);
        
        wallet.push_back({value, available});
    }
    
    // Поиск amount
    size_t amountPos = content.find("\"amount\"");
    if (amountPos == string::npos) return false;
    
    size_t colonPos = content.find(':', amountPos);
    if (colonPos == string::npos) return false;
    
    size_t numStart = colonPos + 1;
    while (numStart < content.size() && !isdigit(content[numStart])) numStart++;
    if (numStart >= content.size()) return false;
    
    amount = 0;
    while (numStart < content.size() && isdigit(content[numStart])) {
        amount = amount * 10 + (content[numStart] - '0');
        numStart++;
    }
    
    // Поиск strategy
    size_t strategyPos = content.find("\"strategy\"");
    if (strategyPos == string::npos) return false;
    
    colonPos = content.find(':', strategyPos);
    if (colonPos == string::npos) return false;
    
    size_t quoteStart = content.find('"', colonPos);
    if (quoteStart == string::npos) return false;
    
    size_t quoteEnd = content.find('"', quoteStart + 1);
    if (quoteEnd == string::npos) return false;
    
    strategy = content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    
    return true;
}

// Структура для хранения временного решения
struct TempSolution {
    vector<int> counts;
    int sum;
};

TempSolution bestSolution;
int bestDiff = INT_MAX;

// Сравнение решений для MAX стратегии
bool isBetterMAX(const vector<int>& current, const vector<int>& best) {
    if (best.empty()) return true;
    
    // Сортируем номиналы для сравнения по старшинству
    vector<pair<int, int>> curr, bestVec;
    for (size_t i = 0; i < wallet.size(); i++) {
        if (current[i] > 0) curr.push_back({wallet[i].value, current[i]});
        if (best[i] > 0) bestVec.push_back({wallet[i].value, best[i]});
    }
    
    sort(curr.begin(), curr.end());
    sort(bestVec.begin(), bestVec.end());
    
    // Сравниваем с конца (старшие номиналы)
    for (int i = curr.size() - 1, j = bestVec.size() - 1; i >= 0 || j >= 0; i--, j--) {
        int currVal = (i >= 0) ? curr[i].first : 0;
        int bestVal = (j >= 0) ? bestVec[j].first : 0;
        
        if (currVal != bestVal) {
            return currVal > bestVal;
        }
        
        int currCount = (i >= 0) ? curr[i].second : 0;
        int bestCount = (j >= 0) ? bestVec[j].second : 0;
        
        if (currCount != bestCount) {
            return currCount > bestCount;
        }
    }
    
    return false;
}

// Сравнение решений для MIN стратегии
bool isBetterMIN(const vector<int>& current, const vector<int>& best) {
    if (best.empty()) return true;
    
    // Сортируем номиналы
    vector<pair<int, int>> curr, bestVec;
    for (size_t i = 0; i < wallet.size(); i++) {
        if (current[i] > 0) curr.push_back({wallet[i].value, current[i]});
        if (best[i] > 0) bestVec.push_back({wallet[i].value, best[i]});
    }
    
    sort(curr.begin(), curr.end());
    sort(bestVec.begin(), bestVec.end());
    
    // Сравниваем с начала (младшие номиналы)
    for (size_t i = 0; i < max(curr.size(), bestVec.size()); i++) {
        int currVal = (i < curr.size()) ? curr[i].first : INT_MAX;
        int bestVal = (i < bestVec.size()) ? bestVec[i].first : INT_MAX;
        
        if (currVal != bestVal) {
            return currVal < bestVal;
        }
        
        int currCount = (i < curr.size()) ? curr[i].second : 0;
        int bestCount = (i < bestVec.size()) ? bestVec[i].second : 0;
        
        if (currCount != bestCount) {
            return currCount > bestCount;
        }
    }
    
    return false;
}

// Вычисление разницы между max и min количеством для UNIFORM
int calculateDiff(const vector<int>& counts) {
    int maxCount = 0;
    int minCount = INT_MAX;
    
    for (size_t i = 0; i < counts.size(); i++) {
        if (counts[i] > maxCount) maxCount = counts[i];
        if (counts[i] < minCount) minCount = counts[i];
    }
    
    return maxCount - minCount;
}

// Сравнение решений для UNIFORM стратегии
bool isBetterUNIFORM(const vector<int>& current, const vector<int>& best) {
    if (best.empty()) return true;
    
    int currDiff = calculateDiff(current);
    int bestDiff = calculateDiff(best);
    
    if (currDiff != bestDiff) {
        return currDiff < bestDiff;
    }
    
    // При равной разнице выбираем то, где больше общее количество купюр
    int currTotal = 0, bestTotal = 0;
    for (size_t i = 0; i < current.size(); i++) {
        currTotal += current[i];
        bestTotal += best[i];
    }
    
    return currTotal > bestTotal;
}

// DFS для поиска всех решений
void dfs(int idx, int remaining, vector<int>& currentCounts) {
    if (remaining == 0) {
        // Нашли решение
        if (strategy == "MAX") {
            if (isBetterMAX(currentCounts, bestSolution.counts)) {
                bestSolution.counts = currentCounts;
                bestSolution.sum = amount;
            }
        } else if (strategy == "MIN") {
            if (isBetterMIN(currentCounts, bestSolution.counts)) {
                bestSolution.counts = currentCounts;
                bestSolution.sum = amount;
            }
        } else if (strategy == "UNIFORM") {
            if (isBetterUNIFORM(currentCounts, bestSolution.counts)) {
                bestSolution.counts = currentCounts;
                bestSolution.sum = amount;
            }
        }
        return;
    }
    
    if (idx >= (int)wallet.size()) return;
    
    // Максимально возможное количество для текущего номинала
    int maxTake = min(wallet[idx].available, remaining / wallet[idx].value);
    
    // Перебираем все варианты
    for (int take = 0; take <= maxTake; take++) {
        currentCounts[idx] = take;
        dfs(idx + 1, remaining - take * wallet[idx].value, currentCounts);
    }
    
    currentCounts[idx] = 0; // Backtrack
}

int main() {
    // Парсим входной файл
    if (!parseJSON("input.json")) {
        cerr << "Failed to parse input.json" << endl;
        return 1;
    }
    
    // Проверка входных данных
    if (wallet.empty()) {
        cerr << "Wallet is empty" << endl;
        return 1;
    }
    
    // Инициализация
    vector<int> currentCounts(wallet.size(), 0);
    bestSolution.counts.clear();
    bestSolution.sum = 0;
    
    // Запуск DFS
    dfs(0, amount, currentCounts);
    
    // Формирование результата
    if (!bestSolution.counts.empty()) {
        for (size_t i = 0; i < bestSolution.counts.size(); i++) {
            if (bestSolution.counts[i] > 0) {
                bestDispense.push_back({wallet[i].value, bestSolution.counts[i]});
            }
        }
        found = true;
    }
    
    // Запись результата
    ofstream out("output.json");
    if (!out.is_open()) {
        cerr << "Cannot create output.json" << endl;
        return 1;
    }
    
    out << "[\n{\n\"dispense\": [";
    if (found && !bestDispense.empty()) {
        for (size_t i = 0; i < bestDispense.size(); i++) {
            out << "[" << bestDispense[i].first << ", " << bestDispense[i].second << "]";
            if (i != bestDispense.size() - 1) out << ", ";
        }
    }
    out << "]\n}\n]\n";
    
    out.close();
    
    return 0;
}






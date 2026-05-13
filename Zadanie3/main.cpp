#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>

using namespace std;

// Struktura przechowująca parametry zadania
struct Task {
    int id;
    int p; // czas wykonania [cite: 25]
    int w; // waga/współczynnik kary [cite: 26]
    int d; // żądany termin zakończenia [cite: 26]
};

// Funkcja pomocnicza symulująca nextInt(min, max)
int nextInt(mt19937& gen, int min_val, int max_val) {
    uniform_int_distribution<> dist(min_val, max_val);
    return dist(gen);
}

// Metoda generowania instancji zgodnie z sekcją 5 [cite: 60, 61]
vector<Task> generateInstances(int n, int seed, bool x_is_A) {
    mt19937 gen(seed); // 1. init(Z) [cite: 62]
    vector<Task> tasks(n);
    int A = 0;

    // 2. Dla każdego j: p_j = nextInt(1, 29) [cite: 63, 64]
    for (int i = 0; i < n; ++i) {
        tasks[i].id = i + 1;
        tasks[i].p = nextInt(gen, 1, 29);
        A += tasks[i].p; // 3. A = suma p_i [cite: 65]
    }

    // 4. Dla każdego j: w_j = nextInt(1, 9) [cite: 67, 68]
    for (int i = 0; i < n; ++i) {
        tasks[i].w = nextInt(gen, 1, 9);
    }

    // Ustalenie parametru X 
    int X = x_is_A ? A : 29;

    // 5. Dla każdego j: d_j = nextInt(1, X) [cite: 69, 70]
    for (int i = 0; i < n; ++i) {
        tasks[i].d = nextInt(gen, 1, X);
    }

    return tasks;
}

// Funkcja obliczająca sumę czasów zadań w danym podproblemie z wykorzystaniem operacji bitowych
long long calculate_sum_p(int mask, const vector<Task>& tasks) {
    long long sum = 0;
    for (int i = 0; i < tasks.size(); ++i) {
        if ((mask >> i) & 1) { // Sprawdzenie czy zadanie jest w zbiorze
            sum += tasks[i].p;
        }
    }
    return sum;
}

// Główna funkcja rozwiązująca problem 1||Sum w_j T_j
void solve(const vector<Task>& tasks) {
    int n = tasks.size();
    int num_subproblems = 1 << n; // 2^n podproblemów, przesunięcie bitowe 
    
    // Inicjalizacja tablicy memory o rozmiarze 2^n [cite: 110, 112]
    vector<long long> memory(num_subproblems, LLONG_MAX);
    vector<int> last_task(num_subproblems, -1); 

    memory[0] = 0; // Wartość dla pustego zbioru wynosi 0

    // Iteracyjne programowanie dynamiczne 
    for (int mask = 1; mask < num_subproblems; ++mask) {
        long long current_sum_p = calculate_sum_p(mask, tasks); 

        for (int j = 0; j < n; ++j) {
            if ((mask >> j) & 1) { // Jeśli j-te zadanie jest w masce (podproblemie)
                int prev_mask = mask ^ (1 << j); // Bitowa różnica symetryczna - zbiór bez zadania j
                
                // Obliczanie kary dla zadania j jako ostatniego: max{sum - d_j, 0} * w_j + memory(D \ {j})
                long long tardiness = max(0LL, current_sum_p - tasks[j].d);
                long long current_cost = tardiness * tasks[j].w + memory[prev_mask];

                // Zapamiętywanie minimum dla podproblemu 
                if (current_cost < memory[mask]) {
                    memory[mask] = current_cost;
                    last_task[mask] = j; // Ślad do odtworzenia kolejności (Backtracking)
                }
            }
        }
    }

    cout << "Minimalna wazona suma opoznien (F): " << memory[num_subproblems - 1] << endl;

    // Odtwarzanie kolejności (Backtracking) 
    vector<int> schedule;
    int curr_mask = num_subproblems - 1;
    while (curr_mask > 0) {
        int task_idx = last_task[curr_mask];
        schedule.push_back(tasks[task_idx].id);
        curr_mask ^= (1 << task_idx); // Usuwamy zadanie ze zbioru
    }
    reverse(schedule.begin(), schedule.end());

    cout << "Optymalna kolejnosc zadan (pi): ";
    for (int id : schedule) cout << id << " ";
    cout << "\n----------------------------------------\n";
}

void printTasks(const vector<Task>& tasks) {
    cout << "Wygenerowane zadania (id: p, w, d):\n";
    for (const auto& t : tasks) {
        cout << "Zadanie " << t.id << ": p=" << t.p << ", w=" << t.w << ", d=" << t.d << "\n";
    }
}

int main() {
    int n, seed;
    cout << "Podaj liczbe zadan (n): ";
    cin >> n;
    cout << "Podaj ziarno generatora (Z): ";
    cin >> seed;

    cout << "\n=== TEST 1: Zakres zadan d_j = X = A ===\n";
    vector<Task> tasks_A = generateInstances(n, seed, true);
    printTasks(tasks_A);
    solve(tasks_A);

    cout << "\n=== TEST 2: Zakres zadan d_j = X = 29 ===\n";
    vector<Task> tasks_29 = generateInstances(n, seed, false);
    printTasks(tasks_29);
    solve(tasks_29);

    return 0;
}

#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <random>

using namespace std;

// Struktura przechowująca parametry zadania
struct Task {
    int id;
    int p; // czas wykonania 
    int w; // waga/współczynnik kary 
    int d; // żądany termin zakończenia 
};

// Funkcja pomocnicza symulująca nextInt(min, max)
int nextInt(mt19937& gen, int min_val, int max_val) {
    uniform_int_distribution<> dist(min_val, max_val);
    return dist(gen);
}

// Metoda generowania instancji zgodnie z sekcją 5 
vector<Task> generateInstances(int n, int seed, bool x_is_A) {
    mt19937 gen(seed);
    vector<Task> tasks(n);
    int A = 0;

    for (int i = 0; i < n; ++i) {
        tasks[i].id = i + 1;
        tasks[i].p = nextInt(gen, 1, 29);
        A += tasks[i].p;
    }

    for (int i = 0; i < n; ++i) {
        tasks[i].w = nextInt(gen, 1, 9);
    }

    int X = x_is_A ? A : 29;

    for (int i = 0; i < n; ++i) {
        tasks[i].d = nextInt(gen, 1, X);
    }

    return tasks;
}

// --- ALGORYTM ZACHŁANNY ---
// Sortowanie po żądanych terminach zakończenia
void solve_greedy(vector<Task> tasks) {
    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.d < b.d; 
    });

    long long current_time = 0;
    long long current_cost = 0;
    vector<int> schedule;

    for (const auto& task : tasks) {
        current_time += task.p;
        long long tardiness = max(0LL, current_time - task.d);
        current_cost += tardiness * task.w;
        schedule.push_back(task.id);
    }

    cout << "--- METODA ZACHLANNA ---\n";
    cout << "Minimalna wazona suma opoznien (F): " << current_cost << "\n";
    cout << "Optymalna kolejnosc zadan (pi): ";
    for (int id : schedule) cout << id << " ";
    cout << "\n\n";
}

// --- PRZEGLĄD ZUPEŁNY (BRUTE FORCE) ---
// Sprawdzenie wszystkich możliwych kombinacji
void solve_brute_force(vector<Task> tasks) {
    sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.id < b.id;
    });

    long long min_cost = LLONG_MAX;
    vector<int> best_schedule;

    do {
        long long current_time = 0;
        long long current_cost = 0;

        for (const auto& task : tasks) {
            current_time += task.p;
            long long tardiness = max(0LL, current_time - task.d);
            current_cost += tardiness * task.w;
        }

        if (current_cost < min_cost) {
            min_cost = current_cost;
            best_schedule.clear();
            for (const auto& task : tasks) {
                best_schedule.push_back(task.id);
            }
        }
    } while (next_permutation(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.id < b.id;
    }));

    cout << "--- PRZEGLAD ZUPELNY (Brute Force) ---\n";
    cout << "Minimalna wazona suma opoznien (F): " << min_cost << "\n";
    cout << "Optymalna kolejnosc zadan (pi): ";
    for (int id : best_schedule) cout << id << " ";
    cout << "\n\n";
}

// --- PROGRAMOWANIE DYNAMICZNE ---
long long calculate_sum_p(int mask, const vector<Task>& tasks) {
    long long sum = 0;
    for (int i = 0; i < tasks.size(); ++i) {
        if ((mask >> i) & 1) { 
            sum += tasks[i].p;
        }
    }
    return sum;
}

void solve_dp(const vector<Task>& tasks) {
    int n = tasks.size();
    int num_subproblems = 1 << n; 
    
    vector<long long> memory(num_subproblems, LLONG_MAX);
    vector<int> last_task(num_subproblems, -1); 

    memory[0] = 0; 

    for (int mask = 1; mask < num_subproblems; ++mask) {
        long long current_sum_p = calculate_sum_p(mask, tasks); 

        for (int j = 0; j < n; ++j) {
            if ((mask >> j) & 1) { 
                int prev_mask = mask ^ (1 << j); 
                
                long long tardiness = max(0LL, current_sum_p - tasks[j].d);
                long long current_cost = tardiness * tasks[j].w + memory[prev_mask];

                if (current_cost < memory[mask]) {
                    memory[mask] = current_cost;
                    last_task[mask] = j; 
                }
            }
        }
    }

    cout << "--- PROGRAMOWANIE DYNAMICZNE ---\n";
    cout << "Minimalna wazona suma opoznien (F): " << memory[num_subproblems - 1] << "\n";

    vector<int> schedule;
    int curr_mask = num_subproblems - 1;
    while (curr_mask > 0) {
        int task_idx = last_task[curr_mask];
        schedule.push_back(tasks[task_idx].id);
        curr_mask ^= (1 << task_idx); 
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
    cout << "\n";
}

int main() {
    int n, seed;
    cout << "Podaj liczbe zadan (n): ";
    cin >> n;
    cout << "Podaj ziarno generatora (Z): ";
    cin >> seed;

    cout << "\n========================================\n";
    cout << "=== TEST 1: Zakres zadan d_j = X = A ===\n";
    cout << "========================================\n";
    vector<Task> tasks_A = generateInstances(n, seed, true);
    printTasks(tasks_A);
    solve_greedy(tasks_A);
    solve_brute_force(tasks_A);
    solve_dp(tasks_A);

    cout << "\n=========================================\n";
    cout << "=== TEST 2: Zakres zadan d_j = X = 29 ===\n";
    cout << "=========================================\n";
    vector<Task> tasks_29 = generateInstances(n, seed, false);
    printTasks(tasks_29);
    solve_greedy(tasks_29);
    solve_brute_force(tasks_29);
    solve_dp(tasks_29);

    return 0;
}

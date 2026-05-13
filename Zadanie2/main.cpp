#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

using namespace std;

// Generator instancji
vector<vector<int>> generate_instance(int n, int m, int seed) {
    vector<vector<int>> p(m, vector<int>(n));
    mt19937 gen(seed);
    uniform_int_distribution<> dist(1, 29);

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            p[i][j] = dist(gen);
        }
    }
    return p;
}

// Obliczanie Cmax dla zadanej permutacji
int calculate_cmax(const vector<int>& order, const vector<vector<int>>& p) {
    if (order.empty()) return 0;
    int m = p.size();
    int n_current = order.size();
    vector<vector<int>> C(m, vector<int>(n_current, 0));

    for (int j = 0; j < n_current; ++j) {
        int job_idx = order[j];
        for (int i = 0; i < m; ++i) {
            int prev_machine = (i == 0) ? 0 : C[i - 1][j];
            int prev_job = (j == 0) ? 0 : C[i][j - 1];
            C[i][j] = max(prev_machine, prev_job) + p[i][job_idx];
        }
    }
    return C[m - 1][n_current - 1];
}

// Zmodyfikowany Algorytm Johnsona (obsługa m >= 2 poprzez maszyny wirtualne)
vector<int> johnson(const vector<vector<int>>& p) {
    int m = p.size();
    int n = p[0].size();
    
    // Tworzenie dwóch maszyn wirtualnych
    vector<vector<int>> p_virtual(2, vector<int>(n, 0));
    if (m == 2) {
        p_virtual = p;
    } else {
        for (int j = 0; j < n; ++j) {
            // Maszyna wirtualna 1: suma od 1 do m-1
            for (int i = 0; i < m - 1; ++i) p_virtual[0][j] += p[i][j];
            // Maszyna wirtualna 2: suma od 2 do m
            for (int i = 1; i < m; ++i) p_virtual[1][j] += p[i][j];
        }
    }

    vector<int> order(n);
    vector<bool> visited(n, false);
    int left = 0, right = n - 1;

    // Klasyczny mechanizm Johnsona na maszynach wirtualnych
    for (int step = 0; step < n; ++step) {
        int min_val = 1e9, min_job = -1, min_machine = -1;
        for (int j = 0; j < n; ++j) {
            if (!visited[j]) {
                if (p_virtual[0][j] < min_val) { min_val = p_virtual[0][j]; min_job = j; min_machine = 0; }
                if (p_virtual[1][j] < min_val) { min_val = p_virtual[1][j]; min_job = j; min_machine = 1; }
            }
        }
        visited[min_job] = true;
        if (min_machine == 0) order[left++] = min_job;
        else order[right--] = min_job;
    }
    return order;
}

// Brute Force w formie drzewiastej (bez odcinania)
void bf_tree_recursive(vector<int> pi, vector<int> N, const vector<vector<int>>& p, int& best_cmax, vector<int>& best_pi) {
    if (N.empty()) {
        int cmax = calculate_cmax(pi, p);
        if (cmax < best_cmax) {
            best_cmax = cmax;
            best_pi = pi;
        }
        return;
    }

    for (size_t k = 0; k < N.size(); ++k) {
        vector<int> next_pi = pi;
        next_pi.push_back(N[k]);
        
        vector<int> next_N = N;
        next_N.erase(next_N.begin() + k);

        bf_tree_recursive(next_pi, next_N, p, best_cmax, best_pi);
    }
}

// Wywołanie Brute Force
vector<int> brute_force_tree(const vector<vector<int>>& p, int& best_cmax) {
    int n = p[0].size();
    vector<int> N(n);
    iota(N.begin(), N.end(), 0);
    vector<int> pi, best_pi;
    best_cmax = 1e9;

    bf_tree_recursive(pi, N, p, best_cmax, best_pi);
    return best_pi;
}

// Obliczanie LB3 dla Branch and Bound
int calculate_lb3(const vector<int>& pi, const vector<int>& N, const vector<vector<int>>& p) {
    int m = p.size();
    if (N.empty()) return calculate_cmax(pi, p);

    int n_current = pi.size();
    vector<int> C_last(m, 0); 
    
    // Oblicz czasy zakonczenia aktualnie uszeregowanych zadan
    if (n_current > 0) {
        vector<vector<int>> C(m, vector<int>(n_current, 0));
        for (int j = 0; j < n_current; ++j) {
            int job_idx = pi[j];
            for (int i = 0; i < m; ++i) {
                int prev_m = (i == 0) ? 0 : C[i - 1][j];
                int prev_j = (j == 0) ? 0 : C[i][j - 1];
                C[i][j] = max(prev_m, prev_j) + p[i][job_idx];
            }
        }
        for (int i = 0; i < m; ++i) C_last[i] = C[i][n_current - 1];
    }

    int lb = 0;
    for (int i = 0; i < m; ++i) {
        int sum_p = 0;
        for (int j : N) sum_p += p[i][j];

        int sum_min = 0;
        for (int k = i + 1; k < m; ++k) {
            int min_val = 1e9;
            for (int j : N) {
                if (p[k][j] < min_val) min_val = p[k][j];
            }
            if (min_val != 1e9) sum_min += min_val;
        }

        lb = max(lb, C_last[i] + sum_p + sum_min);
    }
    return lb;
}

// Rekurencja dla Branch and Bound z LB3
void bnb_recursive(vector<int> pi, vector<int> N, const vector<vector<int>>& p, int& ub, vector<int>& best_pi) {
    if (N.empty()) {
        int cmax = calculate_cmax(pi, p);
        if (cmax < ub) {
            ub = cmax;
            best_pi = pi;
        }
        return;
    }

    for (size_t k = 0; k < N.size(); ++k) {
        vector<int> next_pi = pi;
        next_pi.push_back(N[k]);
        
        vector<int> next_N = N;
        next_N.erase(next_N.begin() + k);

        int lb = calculate_lb3(next_pi, next_N, p);
        if (lb < ub) {
            bnb_recursive(next_pi, next_N, p, ub, best_pi);
        }
    }
}

// Wywołanie Branch and Bound
vector<int> branch_and_bound(const vector<vector<int>>& p, int& best_cmax) {
    int n = p[0].size();
    vector<int> N(n);
    iota(N.begin(), N.end(), 0);
    
    // Zmiana optymalizacyjna: Inicjalizacja UB za pomocą heurystyki Johnsona
    vector<int> init_pi = johnson(p);
    best_cmax = calculate_cmax(init_pi, p);
    vector<int> best_pi = init_pi;
    
    vector<int> pi;
    bnb_recursive(pi, N, p, best_cmax, best_pi);
    return best_pi;
}

int main() {
    int n, m, seed;
    
    cout << "Podaj liczbe zadan (n): ";
    if (!(cin >> n)) return 1;
    cout << "Podaj liczbe maszyn (m): ";
    if (!(cin >> m)) return 1;
    cout << "Podaj ziarno losowania (seed): ";
    if (!(cin >> seed)) return 1;

    vector<vector<int>> p = generate_instance(n, m, seed);

    cout << "\nWygenerowana macierz czasow:\n";
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) cout << p[i][j] << "\t";
        cout << "\n";
    }
    
    cout << "\n--- Wyniki ---\n";

    // Johnson teraz działa dla każdego 'm'
    vector<int> order_johnson = johnson(p);
    cout << "Johnson Cmax (Heurystyka):\t" << calculate_cmax(order_johnson, p) << "\n";

    int bf_cmax;
    vector<int> order_bf = brute_force_tree(p, bf_cmax);
    cout << "Brute Force (Tree) Cmax:\t" << bf_cmax << "\n";

    int bnb_cmax;
    vector<int> order_bnb = branch_and_bound(p, bnb_cmax);
    cout << "Branch and Bound Cmax:\t\t" << bnb_cmax << "\n";

    return 0;
}

#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <limits>
#include <queue> 

std::pair<int, std::vector<int>> Schrage(int n, const std::vector<int>& r, const std::vector<int>& p, const std::vector<int>& q);
std::pair<int, std::vector<int>> SchrageHeap(int n, const std::vector<int>& r, const std::vector<int>& p, const std::vector<int>& q);
int SchragePMTN(int n, const std::vector<int>& r, std::vector<int> p, const std::vector<int>& q);
void Carlier(int n, std::vector<int>& r, const std::vector<int>& p, std::vector<int>& q, int& UB, std::vector<int>& pi_star);

void displaySchedule(const std::string& name, int n, const std::vector<int>& pi, const std::vector<int>& r, const std::vector<int>& p, const std::vector<int>& q) {
    std::cout << "\n--- Harmonogram dla: " << name << " ---" << std::endl;
    int current_t = 0;
    int C_max = 0;
    std::cout << "Zadanie\t| r_j\t| p_j\t| q_j\t|| S_j\t| C_j\t| C_j+q_j" << std::endl;
    std::cout << "---------------------------------------------------------" << std::endl;
    for(int i = 0; i < n; ++i) {
        int task = pi[i];
        int S_j = std::max(current_t, r[task]);
        int C_j = S_j + p[task];
        int C_q = C_j + q[task];
        C_max = std::max(C_max, C_q);
        current_t = C_j;
        std::cout << task << "\t| " << r[task] << "\t| " << p[task] << "\t| " << q[task] 
                  << "\t|| " << S_j << "\t| " << C_j << "\t| " << C_q << std::endl;
    }
    std::cout << ">> Ostateczny C_max: " << C_max << std::endl;
}

int main() {
    std::cout << "Enter the seed: ";
    int seed;
    std::cin >> seed;
    srand(seed);

    int n = 0;
    std::cout << "n: ";
    std::cin >> n;
    
    std::vector<int> p(n), r(n), q(n);

    int A = 0;
    for(int i = 0; i < n; i++) {
        p[i] = (rand() % 29) + 1;
        A += p[i];
    }
    
    int X = 29;
    for(int i = 0; i < n; i++) {
        r[i] = (rand() % A) + 1;
        q[i] = (rand() % X) + 1;
    }

    std::cout << "\n--- Wygenerowane instancje ---" << std::endl;
    for(int i = 0; i < n; i++) {
        std::cout << "Zadanie " << i << " -> r: " << r[i] << " p: " << p[i] << " q: " << q[i] << std::endl;
    }

    auto [schrage_Cmax, schrage_pi] = Schrage(n, r, p, q);
    displaySchedule("SCHRAGE (Wersja bazowa)", n, schrage_pi, r, p, q);

    auto [schrageHeap_Cmax, schrageHeap_pi] = SchrageHeap(n, r, p, q);
    displaySchedule("SCHRAGE (Wersja na kopcach)", n, schrageHeap_pi, r, p, q);

    int UB = std::numeric_limits<int>::max(); 
    std::vector<int> pi_star;                
    
    Carlier(n, r, p, q, UB, pi_star);
    displaySchedule("CARLIER (Optymalny)", n, pi_star, r, p, q);

    return 0;
}

// ==========================================
// 1. Zwykly algorytm Schrage - O(n^2)
// ==========================================
std::pair<int, std::vector<int>> Schrage(int n, const std::vector<int>& r, const std::vector<int>& p, const std::vector<int>& q) {
    std::vector<int> G, N(n), pi;
    for (int i = 0; i < n; ++i) N[i] = i;
    
    int t = 0, C_max = 0;

    while (!G.empty() || !N.empty()) {
        if (G.empty()) {
            int min_r = r[N[0]];
            for (int i : N) min_r = std::min(min_r, r[i]);
            t = std::max(t, min_r);
        }
        
        auto it = N.begin();
        while (it != N.end()) {
            if (r[*it] <= t) {
                G.push_back(*it);
                it = N.erase(it);
            } else {
                ++it;
            }
        }
        
        if (!G.empty()) {
            auto max_q_it = G.begin();
            for (auto i = G.begin(); i != G.end(); ++i) {
                if (q[*i] > q[*max_q_it]) max_q_it = i;
            }
            int j = *max_q_it;
            G.erase(max_q_it);
            
            pi.push_back(j);
            t += p[j];
            C_max = std::max(C_max, t + q[j]);
        }
    }
    return {C_max, pi};
}

// ==========================================
// 2. Algorytm Schrage na kopcach (Priority Queue) - O(n log n)
// ==========================================
std::pair<int, std::vector<int>> SchrageHeap(int n, const std::vector<int>& r, const std::vector<int>& p, const std::vector<int>& q) {
    auto comp_r = [&r](int a, int b) { return r[a] > r[b]; }; // Min-heap po 'r'
    auto comp_q = [&q](int a, int b) { return q[a] < q[b]; }; // Max-heap po 'q'

    std::priority_queue<int, std::vector<int>, decltype(comp_r)> N(comp_r);
    std::priority_queue<int, std::vector<int>, decltype(comp_q)> G(comp_q);

    for (int i = 0; i < n; ++i) {
        N.push(i);
    }

    int t = 0, C_max = 0;
    std::vector<int> pi;

    while (!G.empty() || !N.empty()) {
        if (G.empty() && !N.empty() && r[N.top()] > t) {
            t = r[N.top()];
        }

        while (!N.empty() && r[N.top()] <= t) {
            G.push(N.top());
            N.pop();
        }

        if (!G.empty()) {
            int j = G.top();
            G.pop();

            pi.push_back(j);
            t += p[j];
            C_max = std::max(C_max, t + q[j]);
        }
    }
    return {C_max, pi};
}

// ==========================================
// 3. Schrage z przerwaniami (Lower Bound dla Carliera)
// ==========================================
int SchragePMTN(int n, const std::vector<int>& r, std::vector<int> p, const std::vector<int>& q) {
    std::vector<int> G, N(n);
    for (int i = 0; i < n; ++i) N[i] = i;
    
    int t = 0, C_max = 0;

    while (!G.empty() || !N.empty()) {
        if (G.empty()) {
            int min_r = r[N[0]];
            for (int i : N) min_r = std::min(min_r, r[i]);
            t = std::max(t, min_r);
        }
        
        auto it = N.begin();
        while (it != N.end()) {
            if (r[*it] <= t) {
                G.push_back(*it);
                it = N.erase(it);
            } else {
                ++it;
            }
        }
        
        auto max_q_it = G.begin();
        for (auto i = G.begin(); i != G.end(); ++i) {
            if (q[*i] > q[*max_q_it]) max_q_it = i;
        }
        int j = *max_q_it;
        
        int next_arrival = std::numeric_limits<int>::max();
        for (int i : N) next_arrival = std::min(next_arrival, r[i]);
        
        int next_t = t + p[j];
        
        if (next_arrival < next_t) {
            p[j] -= (next_arrival - t);
            t = next_arrival;
        } else {
            t = next_t;
            G.erase(max_q_it);
            C_max = std::max(C_max, t + q[j]);
        }
    }
    return C_max;
}

// ==========================================
// 4. Rekurencyjny Algorytm Carliera
// ==========================================
void Carlier(int n, std::vector<int>& r, const std::vector<int>& p, std::vector<int>& q, int& UB, std::vector<int>& pi_star) {
    auto [U, pi] = Schrage(n, r, p, q);
    
    if (U < UB) {
        UB = U;
        pi_star = pi;
    }

    std::vector<int> C_times(n);
    int current_t = 0;
    for (int i = 0; i < n; ++i) {
        int task = pi[i];
        current_t = std::max(current_t, r[task]) + p[task];
        C_times[task] = current_t;
    }

    int b = -1, a = -1, c = -1;

    for (int i = n - 1; i >= 0; --i) {
        if (U == C_times[pi[i]] + q[pi[i]]) {
            b = i;
            break;
        }
    }

    for (int i = 0; i <= b; ++i) {
        int sum_p = 0;
        for (int k = i; k <= b; ++k) sum_p += p[pi[k]];
        
        if (U == r[pi[i]] + sum_p + q[pi[b]]) {
            a = i;
            break;
        }
    }

    for (int i = b - 1; i >= a; --i) {
        if (q[pi[i]] < q[pi[b]]) {
            c = i;
            break;
        }
    }

    if (c == -1) return;

    int r_hat = std::numeric_limits<int>::max();
    int q_hat = std::numeric_limits<int>::max();
    int p_hat = 0;

    for (int i = c + 1; i <= b; ++i) {
        r_hat = std::min(r_hat, r[pi[i]]);
        q_hat = std::min(q_hat, q[pi[i]]);
        p_hat += p[pi[i]];
    }

    int old_r = r[pi[c]];
    r[pi[c]] = std::max(r[pi[c]], r_hat + p_hat);
    
    int LB_left = SchragePMTN(n, r, p, q);
    if (LB_left < UB) {
        Carlier(n, r, p, q, UB, pi_star);
    }
    r[pi[c]] = old_r; 

    int old_q = q[pi[c]];
    q[pi[c]] = std::max(q[pi[c]], q_hat + p_hat);
    
    int LB_right = SchragePMTN(n, r, p, q);
    if (LB_right < UB) {
        Carlier(n, r, p, q, UB, pi_star);
    }
    q[pi[c]] = old_q; 
}

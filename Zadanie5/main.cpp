#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <limits>

using namespace std;

// =============================================================================
//  Laboratorium 6 - Algorytmy metaheurystyczne dla problemu FP||Cmax.
//
//  Na ocene 5.0 implementujemy DWA algorytmy dla FP||Cmax:
//    - Simulated Annealing (Algorytm 1 z instrukcji),
//    - Tabu Search (Algorytm 2 z instrukcji).
//  Dodatkowo Random Search (baseline) oraz NEH (algorytm konstrukcyjny z Lab 5)
//  sluza do porownania jakosci rozwiazan.
// =============================================================================

// -----------------------------------------------------------------------------
//  Funkcja celu: C_max dla problemu Flow Shop (FP||Cmax).
//  processingTimes[zadanie][maszyna], permutation - kolejnosc zadan.
// -----------------------------------------------------------------------------
int calculateMakespan(const vector<int>& permutation, const vector<vector<int>>& processingTimes) {
    if (permutation.empty()) return 0;

    int numTasks = permutation.size();
    int numMachines = processingTimes[0].size();

    vector<vector<int>> completionTimes(numTasks, vector<int>(numMachines, 0));

    completionTimes[0][0] = processingTimes[permutation[0]][0];
    for (int j = 1; j < numMachines; ++j)
        completionTimes[0][j] = completionTimes[0][j-1] + processingTimes[permutation[0]][j];

    for (int i = 1; i < numTasks; ++i) {
        completionTimes[i][0] = completionTimes[i-1][0] + processingTimes[permutation[i]][0];
        for (int j = 1; j < numMachines; ++j)
            completionTimes[i][j] = max(completionTimes[i-1][j], completionTimes[i][j-1])
                                    + processingTimes[permutation[i]][j];
    }
    return completionTimes[numTasks-1][numMachines-1];
}

void printPermutation(const vector<int>& perm) {
    cout << "[ ";
    for (int p : perm) cout << p + 1 << " ";
    cout << "]";
}

// -----------------------------------------------------------------------------
//  Generator instancji (czasy p_ij z zakresu [1,29], deterministyczny dla ziarna).
// -----------------------------------------------------------------------------
vector<vector<int>> generateInstance(int numTasks, int numMachines, unsigned seed) {
    mt19937 gen(seed);
    uniform_int_distribution<> dist(1, 29);
    vector<vector<int>> p(numTasks, vector<int>(numMachines));
    for (int j = 0; j < numTasks; ++j)
        for (int i = 0; i < numMachines; ++i)
            p[j][i] = dist(gen);
    return p;
}

// =============================================================================
//  Ruchy w przestrzeni rozwiazan (sasiedztwo).
// =============================================================================
// swap - zamiana dwoch zadan miejscami.
vector<int> moveSwap(vector<int> perm, int i, int j) {
    swap(perm[i], perm[j]);
    return perm;
}
// insert - wyjecie zadania z pozycji i i wstawienie na pozycje j.
vector<int> moveInsert(vector<int> perm, int i, int j) {
    int v = perm[i];
    perm.erase(perm.begin() + i);
    perm.insert(perm.begin() + j, v);
    return perm;
}
// reverse - odwrocenie fragmentu permutacji miedzy i a j.
vector<int> moveReverse(vector<int> perm, int i, int j) {
    if (i > j) swap(i, j);
    reverse(perm.begin() + i, perm.begin() + j + 1);
    return perm;
}

// =============================================================================
//  Algorytm konstrukcyjny NEH (z Lab 5) - sluzy jako rozwiazanie startowe
//  oraz punkt odniesienia w porownaniu.
// =============================================================================
vector<int> neh(const vector<vector<int>>& processingTimes) {
    int numTasks = processingTimes.size();
    vector<int> order(numTasks);
    iota(order.begin(), order.end(), 0);

    vector<int> sums(numTasks);
    for (int j = 0; j < numTasks; ++j)
        sums[j] = accumulate(processingTimes[j].begin(), processingTimes[j].end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) { return sums[a] > sums[b]; });

    vector<int> partial;
    for (int job : order) {
        int bestPos = 0, bestCost = numeric_limits<int>::max();
        for (int l = 0; l <= (int)partial.size(); ++l) {
            vector<int> cand = partial;
            cand.insert(cand.begin() + l, job);
            int c = calculateMakespan(cand, processingTimes);
            if (c < bestCost) { bestCost = c; bestPos = l; }
        }
        partial.insert(partial.begin() + bestPos, job);
    }
    return partial;
}

// =============================================================================
//  ALGORYTM 0 (baseline): Random Search.
//  Losuje permutacje przez zadana liczbe iteracji i pamieta najlepsza.
// =============================================================================
vector<int> randomSearch(const vector<vector<int>>& processingTimes, int iterations, mt19937& gen) {
    int numTasks = processingTimes.size();
    vector<int> current(numTasks);
    iota(current.begin(), current.end(), 0);

    vector<int> best = current;
    int bestCost = calculateMakespan(best, processingTimes);

    for (int it = 0; it < iterations; ++it) {
        shuffle(current.begin(), current.end(), gen);
        int c = calculateMakespan(current, processingTimes);
        if (c < bestCost) { bestCost = c; best = current; }
    }
    return best;
}

// =============================================================================
//  ALGORYTM 1: Simulated Annealing (Algorytm 1 z instrukcji).
//  Chlodzenie geometryczne T' = alpha*T, ruch typu swap.
//  Rozwiazanie startowe: wynik NEH (alg. zachlanny).
// =============================================================================
vector<int> simulatedAnnealing(const vector<vector<int>>& processingTimes, mt19937& gen,
                               bool verbose = false) {
    int numTasks = processingTimes.size();

    double T = 100.0, T_end = 0.1, alpha = 0.97;
    int L = 50; // liczba epok (iteracji wewnetrznych)

    uniform_real_distribution<> probDist(0.0, 1.0);
    uniform_int_distribution<> idxDist(0, numTasks - 1);

    vector<int> current = neh(processingTimes); // start: rozwiazanie zachlanne
    int currentCost = calculateMakespan(current, processingTimes);
    vector<int> best = current;
    int bestCost = currentCost;

    while (T > T_end) {
        for (int k = 0; k < L; ++k) {
            int i = idxDist(gen), j = idxDist(gen);
            while (i == j) j = idxDist(gen);

            vector<int> neighbour = moveSwap(current, i, j);
            int newCost = calculateMakespan(neighbour, processingTimes);
            int delta = currentCost - newCost; // >0 oznacza poprawe

            if (delta > 0) {
                current = neighbour; currentCost = newCost;
            } else {
                double p = exp((double)delta / T);
                if (probDist(gen) < p) { current = neighbour; currentCost = newCost; }
            }

            if (currentCost < bestCost) { bestCost = currentCost; best = current; }
        }
        T *= alpha;
    }

    if (verbose) {
        cout << "[SA] C_max = " << bestCost << " | ";
        printPermutation(best); cout << "\n";
    }
    return best;
}

// =============================================================================
//  ALGORYTM 2: Tabu Search (Algorytm 2 z instrukcji).
//  Sasiedztwo: wszystkie zamiany (swap) par pozycji (j,k), j<k.
//  Lista tabu: macierz tabuList[j][k] = numer iteracji, do ktorej ruch zabroniony.
//  Kryterium aspiracji: ruch tabu dopuszczalny, jesli daje wynik lepszy niz globalne optimum.
//  Zawsze przechodzimy do najlepszego dozwolonego sasiada (nawet gorszego).
// =============================================================================
vector<int> tabuSearch(const vector<vector<int>>& processingTimes, int itLimit,
                       int cadence, bool verbose = false) {
    int n = processingTimes.size();

    vector<int> current = neh(processingTimes); // start: rozwiazanie zachlanne
    int currentCost = calculateMakespan(current, processingTimes);
    vector<int> best = current;
    int bestCost = currentCost;

    vector<vector<int>> tabuList(n, vector<int>(n, 0));

    for (int it = 1; it <= itLimit; ++it) {
        int bestNeighCost = numeric_limits<int>::max();
        int bjj = -1, bkk = -1;

        for (int j = 0; j < n; ++j) {
            for (int k = j + 1; k < n; ++k) {
                vector<int> neighbour = moveSwap(current, j, k);
                int c = calculateMakespan(neighbour, processingTimes);

                bool isTabu = tabuList[j][k] >= it;
                bool aspiration = c < bestCost; // przelamanie listy tabu

                if (isTabu && !aspiration) continue;

                if (c < bestNeighCost) {
                    bestNeighCost = c; bjj = j; bkk = k;
                }
            }
        }

        if (bjj == -1) break; // brak dozwolonego ruchu

        // Zawsze wykonujemy ruch (nawet jesli pogarsza).
        current = moveSwap(current, bjj, bkk);
        currentCost = bestNeighCost;
        tabuList[bjj][bkk] = it + cadence; // zabronienie ruchu na 'cadence' iteracji

        if (currentCost < bestCost) { bestCost = currentCost; best = current; }
    }

    if (verbose) {
        cout << "[TS] C_max = " << bestCost << " | ";
        printPermutation(best); cout << "\n";
    }
    return best;
}

// Prosta kadencja zalezna od rozmiaru instancji.
int n_cadence(int n) { return max(3, n / 2); }

// =============================================================================
//  Porownanie wszystkich algorytmow na zadanej instancji.
// =============================================================================
void compareAll(const string& title, const vector<vector<int>>& inst, mt19937& gen) {
    cout << "=== " << title << " (" << inst.size() << " zadan x "
         << inst[0].size() << " maszyn) ===\n";

    vector<int> nehSol = neh(inst);
    vector<int> rsSol  = randomSearch(inst, 5000, gen);
    vector<int> saSol  = simulatedAnnealing(inst, gen);
    vector<int> tsSol  = tabuSearch(inst, 300, n_cadence(inst.size()));

    auto report = [&](const string& name, const vector<int>& sol) {
        cout << "  " << left << setw(26) << name
             << "C_max = " << setw(5) << calculateMakespan(sol, inst) << " | ";
        printPermutation(sol); cout << "\n";
    };

    report("NEH (konstrukcyjny)", nehSol);
    report("Random Search", rsSol);
    report("Simulated Annealing", saSol);
    report("Tabu Search", tsSol);

    int bestVal = min({calculateMakespan(nehSol, inst), calculateMakespan(rsSol, inst),
                       calculateMakespan(saSol, inst), calculateMakespan(tsSol, inst)});
    cout << "  --> Najlepszy C_max: " << bestVal << "\n\n";
}

int main() {
    cout << "=============================================================\n";
    cout << " FP||Cmax - algorytmy metaheurystyczne (SA + Tabu Search)\n";
    cout << "=============================================================\n\n";

    mt19937 gen(12345);

    // Instancja 1: mala, znana z poprzednich zadan.
    vector<vector<int>> inst1 = {
        {4, 3, 7}, {1, 2, 4}, {5, 4, 1}, {2, 6, 3}
    };
    compareAll("Instancja 1 (recznie)", inst1, gen);

    // Instancja 2 i 3: wygenerowane automatycznie (rozne rozmiary).
    vector<vector<int>> inst2 = generateInstance(10, 5, 42);
    compareAll("Instancja 2 (ziarno 42)", inst2, gen);

    vector<vector<int>> inst3 = generateInstance(20, 8, 2026);
    compareAll("Instancja 3 (ziarno 2026)", inst3, gen);

    return 0;
}

#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>

using namespace std;

// =============================================================================
//  Laboratorium 5 - Problem FP||Cmax (permutacyjny problem przeplywowy)
//  Algorytm konstrukcyjny NEH oraz dwie jego modyfikacje (NEH z innym
//  porzadkiem startowym oraz NEH+ z faza ponownego wstawiania).
// =============================================================================

// -----------------------------------------------------------------------------
//  Funkcja celu: C_max dla problemu Flow Shop (FP||Cmax).
//  processingTimes[zadanie][maszyna] - czas operacji p_ij.
//  permutation - kolejnosc wykonywania zadan (ta sama na kazdej maszynie).
//  Wzor: S_i,pi(j) = max(C_{i-1,pi(j)}, C_{i,pi(j-1)}),  C = S + p.
// -----------------------------------------------------------------------------
int calculateMakespan(const vector<int>& permutation, const vector<vector<int>>& processingTimes) {
    if (permutation.empty()) return 0;

    int numTasks = permutation.size();
    int numMachines = processingTimes[0].size();

    // completionTimes[i][j] - moment zakonczenia i-tego zadania w permutacji na maszynie j.
    vector<vector<int>> completionTimes(numTasks, vector<int>(numMachines, 0));

    completionTimes[0][0] = processingTimes[permutation[0]][0];
    for (int j = 1; j < numMachines; ++j) {
        completionTimes[0][j] = completionTimes[0][j-1] + processingTimes[permutation[0]][j];
    }

    for (int i = 1; i < numTasks; ++i) {
        completionTimes[i][0] = completionTimes[i-1][0] + processingTimes[permutation[i]][0];
        for (int j = 1; j < numMachines; ++j) {
            completionTimes[i][j] = max(completionTimes[i-1][j], completionTimes[i][j-1])
                                    + processingTimes[permutation[i]][j];
        }
    }

    return completionTimes[numTasks-1][numMachines-1];
}

// Funkcja pomocnicza do wypisywania permutacji (numeracja zadan od 1).
void printPermutation(const vector<int>& perm) {
    cout << "[ ";
    for (int p : perm) cout << p + 1 << " ";
    cout << "]";
}

// -----------------------------------------------------------------------------
//  Generator instancji problemu.
//  Dla zadanego rozmiaru (n zadan, m maszyn) oraz ziarna Z generujemy
//  deterministycznie macierz czasow p_ij z zakresu [1, 29].
//  Uzywamy generatora mt19937 zainicjowanego ziarnem - powtarzalne wyniki.
// -----------------------------------------------------------------------------
vector<vector<int>> generateInstance(int numTasks, int numMachines, unsigned seed) {
    mt19937 gen(seed);
    uniform_int_distribution<> dist(1, 29);

    vector<vector<int>> processingTimes(numTasks, vector<int>(numMachines));
    for (int j = 0; j < numTasks; ++j)
        for (int i = 0; i < numMachines; ++i)
            processingTimes[j][i] = dist(gen);

    return processingTimes;
}

// -----------------------------------------------------------------------------
//  Znajduje najlepsza pozycje wstawienia zadania 'job' w permutacji czesciowej.
//  Zwraca pare (najlepsza_pozycja, najlepszy_Cmax).
// -----------------------------------------------------------------------------
pair<int,int> findBestInsertion(const vector<int>& partial, int job,
                                const vector<vector<int>>& processingTimes) {
    int bestPos = 0;
    int bestCost = numeric_limits<int>::max();

    for (int l = 0; l <= (int)partial.size(); ++l) {
        vector<int> candidate = partial;
        candidate.insert(candidate.begin() + l, job);
        int cost = calculateMakespan(candidate, processingTimes);
        if (cost < bestCost) {
            bestCost = cost;
            bestPos = l;
        }
    }
    return {bestPos, bestCost};
}

// -----------------------------------------------------------------------------
//  Bazowy szkielet NEH dla zadanego porzadku startowego 'order'.
//  Kolejno wstawiamy zadania z 'order' na wszystkie mozliwe pozycje i
//  wybieramy te, dla ktorej C_max jest najmniejszy (Algorytm 1 z instrukcji).
// -----------------------------------------------------------------------------
vector<int> nehFromOrder(const vector<int>& order, const vector<vector<int>>& processingTimes,
                         bool verbose) {
    vector<int> partial;

    for (int job : order) {
        auto [pos, cost] = findBestInsertion(partial, job, processingTimes);
        partial.insert(partial.begin() + pos, job);
        if (verbose) {
            cout << "  Wstawiono zadanie " << job + 1 << " na pozycji " << pos + 1 << " -> ";
            printPermutation(partial);
            cout << " | C_max = " << cost << "\n";
        }
    }
    return partial;
}

// -----------------------------------------------------------------------------
//  ALGORYTM PODSTAWOWY: NEH.
//  Porzadek startowy: zadania posortowane malejaco wzgledem sumy czasow operacji.
// -----------------------------------------------------------------------------
vector<int> neh(const vector<vector<int>>& processingTimes, bool verbose) {
    int numTasks = processingTimes.size();

    // Suma czasow operacji w kazdym zadaniu (omega_j).
    vector<int> order(numTasks);
    iota(order.begin(), order.end(), 0);

    vector<int> sums(numTasks);
    for (int j = 0; j < numTasks; ++j)
        sums[j] = accumulate(processingTimes[j].begin(), processingTimes[j].end(), 0);

    sort(order.begin(), order.end(), [&](int a, int b) { return sums[a] > sums[b]; });

    if (verbose) {
        cout << "[NEH] Porzadek startowy (wg malejacej sumy operacji): ";
        printPermutation(order);
        cout << "\n";
    }

    return nehFromOrder(order, processingTimes, verbose);
}

// -----------------------------------------------------------------------------
//  MODYFIKACJA 1: NEH z alternatywnym porzadkiem startowym.
//  Zamiast samej sumy operacji uzywamy kryterium srednia + odchylenie
//  standardowe czasow operacji w zadaniu (znana, skuteczna modyfikacja NEH).
//  Zadania o wiekszej zmiennosci czasow wstawiane sa wczesniej.
// -----------------------------------------------------------------------------
vector<int> nehModInitialOrder(const vector<vector<int>>& processingTimes, bool verbose) {
    int numTasks = processingTimes.size();
    int numMachines = processingTimes[0].size();

    vector<int> order(numTasks);
    iota(order.begin(), order.end(), 0);

    vector<double> priority(numTasks);
    for (int j = 0; j < numTasks; ++j) {
        double mean = accumulate(processingTimes[j].begin(), processingTimes[j].end(), 0.0) / numMachines;
        double var = 0.0;
        for (int i = 0; i < numMachines; ++i)
            var += (processingTimes[j][i] - mean) * (processingTimes[j][i] - mean);
        var /= numMachines;
        priority[j] = mean + sqrt(var); // srednia + odchylenie standardowe
    }

    sort(order.begin(), order.end(), [&](int a, int b) { return priority[a] > priority[b]; });

    if (verbose) {
        cout << "[NEH mod.1] Porzadek startowy (wg srednia + odch. std.): ";
        printPermutation(order);
        cout << "\n";
    }

    return nehFromOrder(order, processingTimes, verbose);
}

// -----------------------------------------------------------------------------
//  MODYFIKACJA 2: NEH+ - dodatkowa faza ponownego wstawiania.
//  Po zbudowaniu rozwiazania NEH wykonujemy przebiegi, w ktorych wybieramy
//  zadanie wg reguly nr 4 (zadanie, ktorego usuniecie najbardziej zmniejsza
//  C_max), usuwamy je i wstawiamy ponownie na najlepsza pozycje.
//  Powtarzamy do braku poprawy.
// -----------------------------------------------------------------------------
vector<int> nehPlus(const vector<vector<int>>& processingTimes, bool verbose) {
    vector<int> solution = neh(processingTimes, false);
    int currentCost = calculateMakespan(solution, processingTimes);

    if (verbose) {
        cout << "[NEH+] Rozwiazanie bazowe NEH: ";
        printPermutation(solution);
        cout << " | C_max = " << currentCost << "\n";
    }

    int numTasks = solution.size();
    bool improved = true;
    int pass = 1;

    while (improved) {
        improved = false;

        // Regula 4: znajdz zadanie, ktorego usuniecie da najmniejszy C_max.
        int bestJobIdx = -1;
        int bestRemovalCost = numeric_limits<int>::max();
        for (int idx = 0; idx < numTasks; ++idx) {
            vector<int> reduced = solution;
            reduced.erase(reduced.begin() + idx);
            int cost = calculateMakespan(reduced, processingTimes);
            if (cost < bestRemovalCost) {
                bestRemovalCost = cost;
                bestJobIdx = idx;
            }
        }

        int job = solution[bestJobIdx];
        vector<int> reduced = solution;
        reduced.erase(reduced.begin() + bestJobIdx);

        // Wstaw ponownie na najlepsza pozycje.
        auto [pos, cost] = findBestInsertion(reduced, job, processingTimes);
        reduced.insert(reduced.begin() + pos, job);

        if (cost < currentCost) {
            solution = reduced;
            currentCost = cost;
            improved = true;
            if (verbose) {
                cout << "  Przebieg " << pass << ": przeniesiono zadanie " << job + 1
                     << " na pozycje " << pos + 1 << " -> ";
                printPermutation(solution);
                cout << " | C_max = " << currentCost << "\n";
            }
        }
        ++pass;
    }

    return solution;
}

// -----------------------------------------------------------------------------
//  Pomocnicze: uruchom algorytm, wypisz wynik i zwroc C_max.
// -----------------------------------------------------------------------------
int runAndReport(const string& name, const vector<int>& solution,
                 const vector<vector<int>>& processingTimes) {
    int cost = calculateMakespan(solution, processingTimes);
    cout << name << " -> C_max = " << cost << " | kolejnosc: ";
    printPermutation(solution);
    cout << "\n";
    return cost;
}

int main() {
    cout << "=============================================================\n";
    cout << " Problem FP||Cmax - algorytm konstrukcyjny NEH i modyfikacje\n";
    cout << "=============================================================\n\n";

    // --- Przyklad 1: mala instancja (ta sama co w Zadaniu 4) ---
    vector<vector<int>> instance1 = {
        {4, 3, 7},
        {1, 2, 4},
        {5, 4, 1},
        {2, 6, 3}
    };

    cout << "### INSTANCJA 1 (4 zadania x 3 maszyny) ###\n\n";

    cout << "--- NEH (przebieg szczegolowy) ---\n";
    vector<int> sol1 = neh(instance1, true);
    cout << "\n";

    cout << "--- NEH modyfikacja 1 (porzadek startowy: srednia + odch. std.) ---\n";
    vector<int> sol1m1 = nehModInitialOrder(instance1, true);
    cout << "\n";

    cout << "--- NEH+ modyfikacja 2 (ponowne wstawianie, regula 4) ---\n";
    vector<int> sol1m2 = nehPlus(instance1, true);
    cout << "\n";

    cout << "Podsumowanie instancji 1:\n";
    runAndReport("  NEH        ", sol1, instance1);
    runAndReport("  NEH mod.1  ", sol1m1, instance1);
    runAndReport("  NEH+ mod.2 ", sol1m2, instance1);
    cout << "\n";

    // --- Przyklad 2: instancja wygenerowana automatycznie ---
    int n = 8, m = 4;
    unsigned seed = 42;
    vector<vector<int>> instance2 = generateInstance(n, m, seed);

    cout << "### INSTANCJA 2 (wygenerowana: " << n << " zadan x " << m
         << " maszyn, ziarno = " << seed << ") ###\n";
    cout << "Macierz czasow p_ij (wiersze = zadania, kolumny = maszyny):\n";
    for (int j = 0; j < n; ++j) {
        cout << "  Z" << j + 1 << ": ";
        for (int i = 0; i < m; ++i) cout << setw(3) << instance2[j][i] << " ";
        cout << "\n";
    }
    cout << "\n";

    cout << "Podsumowanie instancji 2:\n";
    int c0 = runAndReport("  NEH        ", neh(instance2, false), instance2);
    int c1 = runAndReport("  NEH mod.1  ", nehModInitialOrder(instance2, false), instance2);
    int c2 = runAndReport("  NEH+ mod.2 ", nehPlus(instance2, false), instance2);

    cout << "\nNajlepszy wynik instancji 2: C_max = " << min({c0, c1, c2}) << "\n";

    return 0;
}

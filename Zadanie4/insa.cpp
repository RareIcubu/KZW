#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <queue>
#include <limits>

using namespace std;

// =============================================================================
//  Laboratorium 5 - Problem J||Cmax (problem gniazdowy / job shop)
//  Algorytm konstrukcyjny INSA (wariant na ocene 5.0).
//
//  Reprezentacja: krotka permutacji pi = (pi_1, ..., pi_m) (sekcja 2.2.1),
//  gdzie pi_i to kolejnosc wykonywania operacji na i-tej maszynie.
//  Kazde zadanie ma wlasna marszrute technologiczna oraz wlasna liczbe operacji.
// =============================================================================

// Pojedyncza operacja o globalnym identyfikatorze.
struct Op {
    int job;        // do ktorego zadania nalezy
    int idxInJob;   // ktora to operacja w ramach zadania (porzadek technologiczny)
    int machine;    // maszyna, na ktorej musi sie wykonac (0-indeksowana)
    int time;       // czas wykonania p
};

// Instancja problemu J||Cmax.
struct Instance {
    int numTasks;
    int numMachines;
    vector<Op> ops;                 // wszystkie operacje (globalna numeracja)
    vector<vector<int>> jobOps;     // jobOps[j] = id operacji zadania j w porzadku technologicznym
    vector<int> jobPrev;            // jobPrev[op] = poprzednia operacja tego samego zadania (-1 jesli pierwsza)
};

// -----------------------------------------------------------------------------
//  Generator instancji J||Cmax (sekcja 4 instrukcji).
//  o_j     = nextInt(1, floor(m * 1.2))   - liczba operacji zadania
//  p_{k,j} = nextInt(1, 29)               - czas operacji
//  mu_{k,j}= nextInt(1, m)                - maszyna operacji
// -----------------------------------------------------------------------------
Instance generateInstance(int numTasks, int numMachines, unsigned seed) {
    mt19937 gen(seed);
    int maxOps = (int)floor(numMachines * 1.2);
    if (maxOps < 1) maxOps = 1;

    uniform_int_distribution<> opsDist(1, maxOps);
    uniform_int_distribution<> timeDist(1, 29);
    uniform_int_distribution<> machDist(1, numMachines); // 1..m

    Instance inst;
    inst.numTasks = numTasks;
    inst.numMachines = numMachines;
    inst.jobOps.resize(numTasks);

    int globalId = 0;
    for (int j = 0; j < numTasks; ++j) {
        int o_j = opsDist(gen);
        for (int k = 0; k < o_j; ++k) {
            Op op;
            op.job = j;
            op.idxInJob = k;
            op.time = timeDist(gen);
            op.machine = machDist(gen) - 1; // na 0-indeksowane
            inst.ops.push_back(op);
            inst.jobOps[j].push_back(globalId);
            inst.jobPrev.push_back(k == 0 ? -1 : globalId - 1);
            ++globalId;
        }
    }
    return inst;
}

// -----------------------------------------------------------------------------
//  Funkcja celu: C_max dla czesciowego/pelnego harmonogramu.
//  Budujemy graf nastepstw (disjunctive graph z ustalonymi kolejnosciami):
//    - krawedzie porzadku technologicznego (operacja k-1 -> k w zadaniu),
//    - krawedzie kolejnosci na maszynie (z machineSeq).
//  C_max = najdluzsza sciezka. Wykrywamy cykl (rozwiazanie niedopuszczalne).
//  Zwraca C_max lub -1, jesli harmonogram jest niedopuszczalny (cykl).
//  Uwzgledniane sa tylko operacje oznaczone jako wstawione (inserted).
// -----------------------------------------------------------------------------
int computeMakespan(const Instance& inst,
                    const vector<vector<int>>& machineSeq,
                    const vector<char>& inserted) {
    int n = inst.ops.size();
    vector<vector<int>> succ(n);
    vector<int> indeg(n, 0);

    int insertedCount = 0;
    for (int o = 0; o < n; ++o) if (inserted[o]) ++insertedCount;

    // Krawedzie porzadku technologicznego.
    for (int o = 0; o < n; ++o) {
        if (!inserted[o]) continue;
        int jp = inst.jobPrev[o];
        if (jp != -1 && inserted[jp]) {
            succ[jp].push_back(o);
            indeg[o]++;
        }
    }
    // Krawedzie kolejnosci na maszynach.
    for (const auto& seq : machineSeq)
        for (int i = 1; i < (int)seq.size(); ++i) {
            succ[seq[i-1]].push_back(seq[i]);
            indeg[seq[i]]++;
        }

    // Najdluzsza sciezka algorytmem Kahna (sortowanie topologiczne).
    vector<int> start(n, 0);
    queue<int> q;
    for (int o = 0; o < n; ++o)
        if (inserted[o] && indeg[o] == 0) q.push(o);

    int processed = 0, makespan = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        ++processed;
        int finish = start[u] + inst.ops[u].time;
        makespan = max(makespan, finish);
        for (int v : succ[u]) {
            start[v] = max(start[v], finish);
            if (--indeg[v] == 0) q.push(v);
        }
    }

    if (processed != insertedCount) return -1; // cykl -> niedopuszczalne
    return makespan;
}

// -----------------------------------------------------------------------------
//  ALGORYTM INSA dla J||Cmax.
//  Analogicznie do NEH: zadania bierzemy w kolejnosci malejacej sumy czasow
//  operacji. Operacje kazdego zadania wstawiamy w porzadku technologicznym,
//  niezaleznie na wszystkich mozliwych pozycjach na odpowiedniej maszynie,
//  wybierajac pozycje dajaca najmniejszy (dopuszczalny) C_max.
// -----------------------------------------------------------------------------
vector<vector<int>> insa(const Instance& inst, bool verbose) {
    int n = inst.ops.size();
    vector<vector<int>> machineSeq(inst.numMachines);
    vector<char> inserted(n, 0);

    // Porzadek zadan wg malejacej sumy czasow operacji (omega_j).
    vector<int> order(inst.numTasks);
    iota(order.begin(), order.end(), 0);
    vector<int> sums(inst.numTasks, 0);
    for (int j = 0; j < inst.numTasks; ++j)
        for (int op : inst.jobOps[j]) sums[j] += inst.ops[op].time;
    sort(order.begin(), order.end(), [&](int a, int b) { return sums[a] > sums[b]; });

    if (verbose) {
        cout << "[INSA] Porzadek zadan (wg malejacej sumy operacji): [ ";
        for (int j : order) cout << j + 1 << " ";
        cout << "]\n";
    }

    for (int job : order) {
        for (int op : inst.jobOps[job]) {
            int m = inst.ops[op].machine;
            auto& seq = machineSeq[m];

            int bestPos = -1;
            int bestCost = numeric_limits<int>::max();

            inserted[op] = 1;
            // Probujemy wszystkie pozycje na maszynie m.
            for (int l = 0; l <= (int)seq.size(); ++l) {
                seq.insert(seq.begin() + l, op);
                int cost = computeMakespan(inst, machineSeq, inserted);
                seq.erase(seq.begin() + l);
                if (cost != -1 && cost < bestCost) { // tylko dopuszczalne pozycje
                    bestCost = cost;
                    bestPos = l;
                }
            }
            // Awaryjnie (gdyby zadna pozycja nie byla dopuszczalna) - na koniec.
            if (bestPos == -1) bestPos = seq.size();

            seq.insert(seq.begin() + bestPos, op);

            if (verbose) {
                cout << "  Wstawiono op (zad " << inst.ops[op].job + 1
                     << ", nr " << inst.ops[op].idxInJob + 1
                     << ", masz " << m + 1 << ", p=" << inst.ops[op].time
                     << ") na pozycji " << bestPos + 1
                     << " | C_max = " << bestCost << "\n";
            }
        }
    }

    return machineSeq;
}

// -----------------------------------------------------------------------------
//  Wypisanie instancji oraz harmonogramu.
// -----------------------------------------------------------------------------
void printInstance(const Instance& inst) {
    cout << "Instancja: " << inst.numTasks << " zadan, "
         << inst.numMachines << " maszyn, " << inst.ops.size() << " operacji.\n";
    for (int j = 0; j < inst.numTasks; ++j) {
        cout << "  Zadanie " << j + 1 << ": ";
        for (int op : inst.jobOps[j])
            cout << "(masz " << inst.ops[op].machine + 1
                 << ", p=" << inst.ops[op].time << ") ";
        cout << "\n";
    }
}

void printSchedule(const Instance& inst, const vector<vector<int>>& machineSeq) {
    for (int m = 0; m < inst.numMachines; ++m) {
        cout << "  Maszyna " << m + 1 << ": ";
        for (int op : machineSeq[m])
            cout << "Z" << inst.ops[op].job + 1 << "." << inst.ops[op].idxInJob + 1 << " ";
        cout << "\n";
    }
}

int main() {
    cout << "=============================================================\n";
    cout << " Problem J||Cmax (gniazdowy) - algorytm konstrukcyjny INSA\n";
    cout << "=============================================================\n\n";

    // --- Instancja przykladowa zdefiniowana recznie ---
    // 3 zadania, 3 maszyny. Marszruty technologiczne sa rozne.
    {
        Instance inst;
        inst.numTasks = 3;
        inst.numMachines = 3;
        // (job, idxInJob, machine, time) - maszyny 0-indeksowane wewnetrznie.
        vector<vector<pair<int,int>>> routes = {
            {{0,3},{1,2},{2,2}},   // Z1: M1(3) -> M2(2) -> M3(2)
            {{0,2},{2,1},{1,4}},   // Z2: M1(2) -> M3(1) -> M2(4)
            {{1,4},{2,3}}          // Z3: M2(4) -> M3(3)
        };
        int gid = 0;
        inst.jobOps.resize(inst.numTasks);
        for (int j = 0; j < inst.numTasks; ++j) {
            for (int k = 0; k < (int)routes[j].size(); ++k) {
                Op op{j, k, routes[j][k].first, routes[j][k].second};
                inst.ops.push_back(op);
                inst.jobOps[j].push_back(gid);
                inst.jobPrev.push_back(k == 0 ? -1 : gid - 1);
                ++gid;
            }
        }

        cout << "### INSTANCJA 1 (definiowana recznie) ###\n";
        printInstance(inst);
        cout << "\n--- INSA (przebieg szczegolowy) ---\n";
        auto sched = insa(inst, true);
        vector<char> all(inst.ops.size(), 1);
        cout << "\nHarmonogram (kolejnosc operacji na maszynach):\n";
        printSchedule(inst, sched);
        cout << "Koncowy C_max = " << computeMakespan(inst, sched, all) << "\n\n";
    }

    // --- Instancja wygenerowana automatycznie (generator z sekcji 4) ---
    {
        int n = 6, m = 4;
        unsigned seed = 2026;
        Instance inst = generateInstance(n, m, seed);

        cout << "### INSTANCJA 2 (wygenerowana, ziarno = " << seed << ") ###\n";
        printInstance(inst);
        cout << "\n--- INSA ---\n";
        auto sched = insa(inst, false);
        vector<char> all(inst.ops.size(), 1);
        cout << "Harmonogram (kolejnosc operacji na maszynach):\n";
        printSchedule(inst, sched);
        cout << "Koncowy C_max = " << computeMakespan(inst, sched, all) << "\n";
    }

    return 0;
}

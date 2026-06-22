#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>

using namespace std;

// Funkcja obliczająca C_max dla problemu Flow Shop (FP||C_max) 
int calculateMakespan(const vector<int>& permutation, const vector<vector<int>>& processingTimes) {
    if (permutation.empty()) return 0;
    
    int numTasks = permutation.size();
    int numMachines = processingTimes[0].size();
    
    vector<vector<int>> completionTimes(numTasks, vector<int>(numMachines, 0));
    
    completionTimes[0][0] = processingTimes[permutation[0]][0];
    for (int j = 1; j < numMachines; ++j) {
        completionTimes[0][j] = completionTimes[0][j-1] + processingTimes[permutation[0]][j];
    }
    
    for (int i = 1; i < numTasks; ++i) {
        completionTimes[i][0] = completionTimes[i-1][0] + processingTimes[permutation[i]][0];
        for (int j = 1; j < numMachines; ++j) {
            completionTimes[i][j] = max(completionTimes[i-1][j], completionTimes[i][j-1]) + processingTimes[permutation[i]][j];
        }
    }
    
    return completionTimes[numTasks-1][numMachines-1];
}

// Funkcja pomocnicza do wypisywania permutacji
void printPermutation(const vector<int>& perm) {
    cout << "[ ";
    for (int p : perm) cout << p + 1 << " ";
    cout << "]";
}

// Główny algorytm Symulowanego Wyżarzania 
void simulatedAnnealing(const vector<vector<int>>& processingTimes, bool verbose = true) {
    int numTasks = processingTimes.size();
    
    // Parametry algorytmu
    double T = 100.0;           // T_0 - temperatura początkowa 
    double T_end = 1.0;         // T_end - warunek zatrzymania 
    double alpha = 0.90;        // Współczynnik chłodzenia geometrycznego 
    int L = 5;                  // Liczba epok (iteracji wewnętrznych) celowo zmniejszona dla czytelności logów 
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> probDist(0.0, 1.0);
    uniform_int_distribution<> indexDist(0, numTasks - 1);

    // Inicjalizacja rozwiązania początkowego 
    vector<int> currentSolution(numTasks);
    iota(currentSolution.begin(), currentSolution.end(), 0);
    int currentCost = calculateMakespan(currentSolution, processingTimes);
    
    vector<int> bestSolution = currentSolution;
    int bestCost = currentCost;

    if (verbose) {
        cout << "--- START ALGORYTMU ---\n";
        cout << "Rozwiazanie poczatkowe: ";
        printPermutation(currentSolution);
        cout << " | C_max = " << currentCost << "\n\n";
    }

    int epochCounter = 1;

    // Pętla główna algorytmu 
    while (T > T_end) {
        if (verbose) {
            cout << "=== EPOKA " << epochCounter << " | Temperatura: " << fixed << setprecision(2) << T << " ===\n";
        }

        for (int k = 0; k < L; ++k) {
            // Losowanie dwóch indeksów do zamiany
            int idx1 = indexDist(gen);
            int idx2 = indexDist(gen);
            while (idx1 == idx2) {
                idx2 = indexDist(gen); 
            }

            // Generowanie sąsiada 
            vector<int> newSolution = currentSolution;
            swap(newSolution[idx1], newSolution[idx2]);
            int newCost = calculateMakespan(newSolution, processingTimes);
            
            int delta = currentCost - newCost;
            
            if (verbose) {
                cout << "  Iter: " << k + 1 << " | Swap(" << idx1 + 1 << ", " << idx2 + 1 << ") -> ";
                printPermutation(newSolution);
                cout << " | Nowy C_max: " << newCost << " | Delta: " << delta << " -> ";
            }

            // Akceptacja lub odrzucenie 
            if (delta > 0) {
                currentSolution = newSolution;
                currentCost = newCost;
                if (verbose) cout << "AKCEPTACJA (Poprawa)\n";
            } else {
                double p = exp(delta / T); // 
                double randomValue = probDist(gen);
                
                if (verbose) cout << "Szansa: " << fixed << setprecision(4) << p << " (Wylosowano: " << randomValue << ") -> ";

                if (randomValue < p) {
                    currentSolution = newSolution;
                    currentCost = newCost;
                    if (verbose) cout << "AKCEPTACJA (Gorsze rozwiazanie)\n";
                } else {
                    if (verbose) cout << "ODRZUCENIE\n";
                }
            }
            
            // Aktualizacja globalnego optimum 
            if (currentCost < bestCost) {
                bestSolution = currentSolution;
                bestCost = currentCost;
                if (verbose) cout << "    *** NOWE NAJLEPSZE ZNALEZIONE: " << bestCost << " ***\n";
            }
        }
        
        // Obniżenie temperatury 
        T = alpha * T;
        epochCounter++;
        if (verbose) cout << "\n";
    }

    cout << "--- KONIEC ALGORYTMU ---\n";
    cout << "Najlepszy znaleziony koszt (C_max): " << bestCost << "\n";
    cout << "Optymalna kolejnosc zadan: ";
    printPermutation(bestSolution);
    cout << "\n";
}

int main() {
    vector<vector<int>> processingTimes = {
        {4, 3, 7}, 
        {1, 2, 4}, 
        {5, 4, 1}, 
        {2, 6, 3}  
    };

    // Uruchamiamy z włączonym logowaniem (verbose = true)
    simulatedAnnealing(processingTimes, true);

    return 0;
}

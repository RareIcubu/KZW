#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm> // Wymagane dla std::max

void calculate(int n, const std::vector<int>& rj, const std::vector<int>& pj, const std::vector<int>& qj);

int main() {
    // Ustawienie ziarna generatora
    std::cout << "Enter the seed: ";
    int seed;
    std::cin >> seed;
    srand(seed);

    // Inicjalizacja problemu
    int n = 0;
    std::cout << "n: ";
    std::cin >> n;
    
    std::vector<int> pj;
    std::vector<int> rj;
    std::vector<int> qj;

    // Generator pj
    for(int i = 0; i < n; i++) {
        pj.push_back((rand() % 29) + 1);
    }

    int A = 0;
    int X = 29;
    for(int i = 0; i < n; i++) {
        A += pj[i];
    }

    // Generator rj oraz qj
    for(int i = 0; i < n; i++) {
        rj.push_back((rand() % A) + 1);
        qj.push_back((rand() % X) + 1); // Brakowało średnika
    }

    // Wypisywanie
    for(int i = 0; i < n; i++) {
        std::cout << "pj: " << pj[i] << " ";
        std::cout << "rj: " << rj[i] << " ";
        std::cout << "qj: " << qj[i] << std::endl;
    }
    std::cout << "A: " << A << std::endl;

    // Wywołanie funkcji - prosto i czytelnie
    calculate(n, rj, pj, qj);

    return 0;
}

void calculate(int n, const std::vector<int>& rj, const std::vector<int>& pj, const std::vector<int>& qj) {
    if (n == 0) return; // Zabezpieczenie przed pustym wejściem

    std::vector<int> S;
    std::vector<int> C;
    int C_max;

    // Inicjalizacja pierwszego zadania
    S.push_back(rj[0]);
    C.push_back(S[0] + pj[0]);
    C_max = C[0] + qj[0];

    // Iteracja po reszcie zadań
    for(int i = 1; i < n; i++) {
        S.push_back(std::max(C[i-1], rj[i]));
        C.push_back(S[i] + pj[i]);
        C_max = std::max(C_max, C[i] + qj[i]);
        
        std::cout << "S" << i << " " << S[i] << " ";
        std::cout << "C" << i << " " << C[i] << std::endl;
    }
    
    // Wypisanie ostatecznego wyniku C_max
    std::cout << "C_max: " << C_max << std::endl;
}

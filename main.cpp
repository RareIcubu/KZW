#include <iostream>
#include <cstdlib>
#include <vector>

int main(){
  srand(time(0));
  
  int n = 0;

  std::cout<<"n: ";
  std::cin>>n;
  std::vector<int> pj;

  for(int i = 0; i < n; i++){
    pj.push_back((rand() % 29) + 1);
  }

  int A = 0;
  for(int i = 0; i < n; i++){
    A += pj[i];
  }

  std::vector<int> rj;

  for(int i = 0; i < n; i++){
      
    rj.push_back((rand() % A) + 1);

  }

  for(int i = 0; i < n; i++){
    std::cout<<"pj: "<<pj[i]<<" ";
    std::cout<<"rj: "<<rj[i]<<std::endl;
  }
  std::cout<<"A: "<<A<<std::endl;

  std::vector<int> S;
  std::vector<int> C;

  S.push_back(rj[0]);
  C.push_back(S[0] + pj[0]);
  
  for(int i = 1; i < n; i++){
    S.push_back(C[i-1] + rj[i]);
    C.push_back(S[i] + pj[i]);
    std::cout<<"S"<<i<<" "<<S[i]<<" ";
    std::cout<<"C"<<i<<" "<<C[i]<<std::endl;
  }

  return 0;
  

}

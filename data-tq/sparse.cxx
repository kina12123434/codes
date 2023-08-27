#include<iostream>
#include<fstream>
#include<cstdlib>
#include<random>
#include"../src/matrix.h"

int main(void){
  
  int dimension=100;
  Vector dim(3);
  dim[0]=10;
  dim[1]=20;
  dim[2]=30;
  //std::string filenameData("multinomial_main.txt");
  //std::string filenameData("spherical_main.txt");
  std::string filenameData("jinnkoudata.dat");

  std::string::size_type filenameDataDotPosition=filenameData.find_last_of(".");
  if(filenameDataDotPosition==std::string::npos){
    std::cerr << "File:" << filenameData
	      << " needs \".\" and filename-extention." << std::endl;
    exit(1);
  }

  std::ifstream ifs(filenameData);
  if(!ifs){
    std::cerr << "File:" << filenameData
	      << " could not open." << std::endl;
    exit(1);
  }

  int data_number, data_dimension;//データ数、次元数読み込み
  ifs >> data_number;
  ifs >> data_dimension;
	
  Matrix Data(data_number,data_dimension);
  
  //データ読み込み
  for(int cnt=0;cnt<data_number;cnt++){
    for(int ell=0;ell<data_dimension;ell++){
      ifs >> Data[cnt][ell];
    }
  }

  //std::string filenameResult=std::string("multinomial_main_sparse.dat");
  std::string filenameResult=std::string("spherical_main_sparse.dat");
  std::ofstream ofs_jinkou(filenameResult);
  ofs_jinkou<< data_number << "\t";
  ofs_jinkou<< dimension << "\t";
  ofs_jinkou<<std::endl;
  for(int cnt=0;cnt<data_number;cnt++){
    ofs_jinkou<< 3 << "\t";
    for(int ell=0;ell<data_dimension;ell++){
      ofs_jinkou<<dim[ell]<< "\t";
      ofs_jinkou<<Data[cnt][ell] << "\t";
    }
    ofs_jinkou<<std::endl;
  }
  ofs_jinkou.close();

  return 0;
}

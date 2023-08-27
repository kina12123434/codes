#include "recom.h"

Recom::Recom(int user,
             int item,
             int user_cen,
             int item_cen,
             int miss,
             int cin_start,
             int cin_end) : SEED(0), Current(0), CCurrent(0), Missing(0),
                            Current_start(cin_start), Current_end(cin_end),
                            SparseIncompleteData(user, item),
                            SparseCorrectData(user, item),
                            NormalizedData(user, item),
                            KessonIndex(miss, 2),
                            Similarity(user, user),
                            Mem(user_cen, user),
                            ItemMem(item_cen, item),
                            aveData(user),
                            resultMAE(METHOD_NUMBER, CLUSTERINGTRIALS),
                            resultFmeasure(METHOD_NUMBER, CLUSTERINGTRIALS),
                            choiceMAE(METHOD_NUMBER, cin_end + 1),
                            choiceFmeasure(METHOD_NUMBER, cin_end + 1),
                            choiceAUC(METHOD_NUMBER, cin_end + 1),
                            aveMAE(0),
                            Prediction(miss),
                            SparseIndex(miss),
                            TP_FN((int)return_max_value() * 10),
                            FP_TN((int)return_max_value() * 10),
                            Obje(CLUSTERINGTRIALS),
                            W(user),
                            aveMAE_h(0),
                            aveAUC_h(0),
                            firstKESSONSeed(0),
                            prevExpSeed(0),
                            Averages(3),
                            bestObj(METHOD_NUMBER, cin_end - cin_start + 1)
{
  for (int i = 0; i < (int)return_max_value() * 10; i++)
  {
    TP_FN[i] = 0.0;
    FP_TN[i] = 0.0;
  }
  for (int i = 0; i < CLUSTERINGTRIALS; i++)
    Obje[i] = DBL_MAX;
}

std::string &Recom::method_name(void)
{
  return METHOD_NAME_recom;
}

Vector Recom::obje(void) const
{
  return Obje;
}

Matrix Recom::kessonindex(void) const
{
  return KessonIndex;
}

Matrix Recom::similarity(void) const
{
  return Similarity;
}

double Recom::similarity(int index1, int index2)
{
  return Similarity[index1][index2];
}

double &Recom::obje(int index)
{
  return Obje[index];
}

int &Recom::current(void)
{
  return Current;
}

int &Recom::current_start(void)
{
  return Current_start;
}

int &Recom::current_end(void)
{
  return Current_end;
}

int &Recom::Ccurrent(void)
{
  return CCurrent;
}

int &Recom::missing(void)
{
  return Missing;
}

std::vector<int> &Recom::FIRST_KESSON_SEED(void)
{
  return firstKESSONSeed;
}

void Recom::input(std::string FileName)
{
  std::ifstream ifs(FileName);
  if (!ifs)
  {
    std::cerr << "DirectoryName" << FileName
              << ": could not open." << std::endl;
    exit(1);
  }
  for (int cnt = 0; cnt < return_user_number(); cnt++)
  {
    int essencialSize;
    ifs >> essencialSize;
    SparseVector dummy(return_item_number(), essencialSize);
    for (int ell = 0; ell < essencialSize; ell++)
    {
      ifs >> dummy.indexIndex(ell) >> dummy.elementIndex(ell);
    }
    SparseCorrectData[cnt] = dummy;
  }
  ifs.close();
}

void Recom::SeedSet1()
{
  //Current += Current_start していない状態
  if(Current == 0){
    if(Current_start == 0){
      SEED = 0;
    } else {
      SEED = prevExpSeed;
    }
  }
  return;
}

void Recom::SeedSet2(){
  if(Missing > KESSON_BEGIN){
    SEED = firstKESSONSeed[Current];
  } else if((signed int)firstKESSONSeed.size() < (Current_end - Current_start + 1)) { // size() は long unsigned int
    firstKESSONSeed.push_back(SEED);
  }
  return;
}

int Recom::loadSEEDandAverage(std::string dir)
{ // Current_start が1以上のときのみこの関数は実行される
  // std::cout << """loadSEEDandAverage"" IS CALLED.\n";
  std::string filename = dir + "/";
  filename += std::to_string(Current_start) + "_SEEDandAverage.txt";
  std::ifstream ifs(filename, std::ios::app);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(loadSEEDandAverage): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    getline(ifs, str);
    prevExpSeed = std::stoi(str);
    getline(ifs, str);
    std::istringstream stream(str); //型合わせ
    int i = 0;
    while(getline(stream, tmp, '\t')){
      Averages[i] = std::stod(tmp);
      i++;
    }
  }
  if(Averages[1] == -1) //前回の実験でNaNが出ていた場合
    return 1;
  return 0;
}

void Recom::loadSEED(std::string dir)
{ // Current_start が1以上のときのみこの関数は実行される
  std::string filename = dir + "/";
  filename += std::to_string(Current_start) + "_SEED.txt";
  std::ifstream ifs(filename, std::ios::app);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(loadSEED): " << filename << std::endl;
  }
  else
  {
    ifs >> prevExpSeed;
  }
  return;
}

void Recom::saveSEEDandAverage(std::string dir, std::string time, bool nan)
{
  std::string filename = dir + "/";
  filename += std::to_string(Current_end+1) + "_SEEDandAverage.txt";
  std::ofstream ofs(filename, std::ios::app);
  if (!ofs)
  {
    std::cerr << "ファイルopen失敗(saveSEEDandAverage): " << filename << std::endl;
  }
  else
  {
    if(nan){
      ofs << 0 << std::endl;
      ofs << DBL_MAX << "\t"
          << -1 << "\t"
          << -1 << "\n"
          << time << std::endl;
    } else {
      int& tmp = firstKESSONSeed.back();
      ofs << tmp << std::endl;
      //MAE, F-measure, AUCの順
      ofs << std::fixed << std::setprecision(10)
          << Averages[0] << "\t"
          << Averages[1] << "\t"
          << Averages[2] << std::endl;
      ofs << time << std::endl;
    }
    ofs.close();
  }
}

void Recom::saveSEED(std::string dir, std::string time)
{
  std::string filename = dir + "/";
  filename += std::to_string(Current_end+1) + "_SEED.txt";
  std::ofstream ofs(filename, std::ios::app);
  if (!ofs) {
    std::cerr << "ファイルopen失敗(saveSEED): " << filename << std::endl;
  } else {
    int& tmp = firstKESSONSeed.back();
    ofs << tmp << std::endl;
    ofs << time << std::endl;
  }
}

void Recom::saveClusterInfo(std::string dir){
  std::string clusterInfo_filename = dir + "/ClusterInfo.txt";
  std::ofstream ofs_cl(clusterInfo_filename);
  if(!ofs_cl){
    std::cerr << "ファイルopen失敗: ClusterInfo.txt" << std::endl;
  }
  ofs_cl << "Cluster\tUser\tItem" << std::endl;
  for(int cluster_index = 0; cluster_index < Mem.rows(); cluster_index++){
    //クラスタに属するユーザ数と，それらが評価したアイテム数
    int cluster_user_num = 0, cluster_item_num = 0;
    //クラスタに属するユーザが評価したアイテムかどうか
    bool isItemEvaluated[return_item_number()];
    for(int i = 0; i < return_item_number(); i++){
      isItemEvaluated[i] = false;
    }
    for(int i = 0; i < return_user_number(); i++){
      if(Mem[cluster_index][i] == 1.0){ //ユーザiがクラスタに属するなら...
        cluster_user_num++;
        for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
          isItemEvaluated[SparseIncompleteData[i].indexIndex(j)] = true;
        }
      }
    }
    for(int i = 0; i < return_item_number(); i++){
      if(isItemEvaluated[i]){
        cluster_item_num++;
      }
    }
    //カウント終了，ファイルへ出力
    ofs_cl << cluster_index + 1 << "\t"
           << cluster_user_num << "\t"
           << cluster_item_num << std::endl;
  }
  ofs_cl.close();
  return;
}

double Recom::choicedAuc_recalculation(int k, std::string dir)
{ 
  std::string filename = dir + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(k) + "sort.txt";
  std::ifstream ifs(filename);
  double rocarea = 0.0;
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(choicedAuc_recalculation): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    int i = 0;
    while(getline(ifs, str)){
    std::istringstream stream(str); //型合わせ
    int j = 0;
    while(getline(stream, tmp, '\t')){
      if(j==0){
        FP_TN[i] = std::stod(tmp);
      }
      else{
        TP_FN[i] = std::stod(tmp);
      }
      j++;
    }
    i++;
   }

    int max = (int)return_max_value() * 10;
    Vector False = FP_TN;
    Vector True = TP_FN;
    Sort(False, True, max);
    for (int i = 0; i < max - 1; i++)
    {
      if ((False[i] < False[i + 1]))
      {
        double low = False[i + 1] - False[i];
        double height = fabs(True[i + 1] - True[i]);
        double squarearea;
        if(True[i] < True[i + 1]){
          squarearea = low * True[i];
        } else {
          squarearea = low * True[i + 1];
        }
        double triangle = (low * height) / 2.0;
        rocarea += squarearea + triangle;
      }
    }
  }

  return rocarea;
}

void Recom::calculationloadsaveChoicedMaeAuc(std::string dir)
{ 
  //読み込み
  std::vector<std::vector<std::string>> retention;
  // retention.resize(Current_end+1);
  // for(int i=0; i < Current_end+1; i++){
  //   retention[i].resize(3);
  // }
  std::string filename =dir + "/ChoicedMaeAuc.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(calculationloadsaveChoicedMaeAuc): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    int i = 0;
    while(getline(ifs, str)){
    retention.resize(retention.size() + 1);
    retention[i].resize(3);
    std::istringstream stream(str); //型合わせ
    int j = 0;
    while(getline(stream, tmp, '\t')){
      retention[i][j] = tmp;
      j++;
    }
    i++;
   }
    //書き込み
    std::ofstream ofs(filename, std::ios::out);
    if (!ofs)
    {
      std::cerr << "ファイルopen失敗(calculationloadsaveChoicedMaeAuc): " << filename << std::endl;
    }
    else
    {   
      for(int i=0; i < Current_end+1; i++){
        //MAE, F-measure, AUCの順
        ofs << retention[i][0] << "\t"
            << retention[i][1] << "\t"
            << std::fixed << std::setprecision(10)
            << choicedAuc_recalculation(i, dir) << std::endl;
      }
      
      ofs.close();
    }
  ifs.close();
  }
  
  return;
}

double Recom::averageAuc_recalculation(std::string dir)
{ 
  // Matrix choiced(Current_end+1, 3);
  std::vector <std::vector <double>> choiced;
  double averagechoiced = 0;
  std::string filename = dir + "/ChoicedMaeAuc.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(averageAuc_recalculation): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    int i = 0;
    while(getline(ifs, str)){
      choiced.resize(choiced.size() + 1);
      choiced[i].resize(3);
      std::istringstream stream(str); //型合わせ
      int j = 0;
      while(getline(stream, tmp, '\t')){
        choiced[i][j] = std::stod(tmp);
        j++;
      }
      i++;
    }

    for(int i=0; i < Current_end+1; i++){
      averagechoiced += choiced[i][2];
    }

    averagechoiced /= (Current_end+1.0);
  }

  return averagechoiced;
}

void Recom::calculationloadsaveSEEDandAverage(std::string dir)
{ 
  //読み込み
  std::vector<std::vector<std::string>> retention;
  retention.resize(3);
  retention[0].resize(1);
  retention[1].resize(3);
  retention[2].resize(1);
  std::string filename = dir + "/";
  filename += std::to_string(Current_end+1) + "_SEEDandAverage.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(calculationloadsaveSEEDandAverage): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    getline(ifs, str);
    retention[0][0] = str;
    getline(ifs, str);
    std::istringstream stream(str); //型合わせ
    int i = 0;
    while(getline(stream, tmp, '\t')){
      retention[1][i] = tmp;
      i++;
    }
    getline(ifs, str);
    retention[2][0] = str;

    if(retention[1][1] != "-1"){
      //書き込み
      std::ofstream ofs(filename, std::ios::out);
      if (!ofs)
      {
        std::cerr << "ファイルopen失敗(calculationloadsaveSEEDandAverage): " << filename << std::endl;
      }
      else
      {   
          ofs << retention[0][0] << std::endl;
          //MAE, F-measure, AUCの順
          ofs << retention[1][0] << "\t"
              << retention[1][1] << "\t"
              << std::fixed << std::setprecision(10)
              << averageAuc_recalculation(dir) << std::endl;
          ofs << retention[2][0] << std::endl;
        
        ofs.close();
      }
    }
  ifs.close();
  }
  
  return;
}

void Recom::calculationloadsaveaverageMaeFmeasureAuc(std::string dir)
{ 
  //読み込み
  std::vector<std::string> retention;
  retention.resize(5);
  std::string filename = dir + "/averageMaeFmeasureAuc.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(calculationloadsaveSEEDandAverage): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    getline(ifs, str);
    std::istringstream stream(str); //型合わせ
    int i = 0;
    while(getline(stream, tmp, '\t')){
      retention[i] = tmp;
      i++;
    }

    //書き込み
    std::ofstream ofs(filename, std::ios::out);
    if (!ofs)
    {
      std::cerr << "ファイルopen失敗(calculationloadsaveaverageMaeFmeasureAuc): " << filename << std::endl;
    }
    else
    {
        ofs << retention[0] << "\t"
            << retention[1] << "\t"
            << retention[2] << "\t"
            << retention[3] << "\t"
            << std::fixed << std::setprecision(10)
            << averageAuc_recalculation(dir) << std::endl;
      ofs.close();
    }
    ifs.close();
  }
  
  return;
}

double Recom::Auc_recalculation(int k, int l, std::string dir)
{ 
  double rocarea = 0.0;
  std::string filename = dir + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(k) + "_" + std::to_string(l) + "sort.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    filename = dir + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(k) + "sort.txt";
     std::ifstream ifs2(filename, std::ios::app);
    if (!ifs2)
    {
      std::cerr << "ファイルopen失敗(Auc_recalculation): " << filename << std::endl;
    } else {   
      std::string str, tmp;
      int i = 0;
      while(getline(ifs2, str)){
        std::istringstream stream(str); //型合わせ
        int j = 0;
        while(getline(stream, tmp, '\t')){
          if(j==0){
            FP_TN[i] = std::stod(tmp);
          }
          else{
            TP_FN[i] = std::stod(tmp);
          }
          j++;
        }
        i++;
      }
    }
  }
  else
  {
    std::string str, tmp;
    int i = 0;
    while(getline(ifs, str)){
      std::istringstream stream(str); //型合わせ
      int j = 0;
      while(getline(stream, tmp, '\t')){
        if(j==0){
          FP_TN[i] = std::stod(tmp);
        }
        else{
          TP_FN[i] = std::stod(tmp);
        }
        j++;
      }
      i++;
    }

    int max = (int)return_max_value() * 10;
    double rocarea = 0.0;
    Vector False = FP_TN;
    Vector True = TP_FN;
    Sort(False, True, max);
    for (int i = 0; i < max - 1; i++)
    {
      if ((False[i] < False[i + 1]))
      {
        double low = False[i + 1] - False[i];
        double height = fabs(True[i + 1] - True[i]);
        double squarearea;
        if(True[i] < True[i + 1]){
          squarearea = low * True[i];
        } else {
          squarearea = low * True[i + 1];
        }
        double triangle = (low * height) / 2.0;
        rocarea += squarearea + triangle;
      }
    }
  }

  return rocarea;
}

void Recom::calculationloadsaveAUC(std::string dir)
{ 
  //読み込み
  std::vector<std::vector<std::string>> retention;
  // retention.resize((Current_end+1)*10);
  // for(int i=0; i < (Current_end+1)*10; i++){
  //   retention[i].resize(5);
  // }
  std::string filename =dir + "/" + METHOD_NAME_recom + "AUC.txt";
  std::ifstream ifs(filename);
  if (!ifs)
  {
    std::cerr << "ファイルopen失敗(calculationloadsaveAUC): " << filename << std::endl;
  }
  else
  {
    std::string str, tmp;
    int i = 0;
    while(getline(ifs, str)){
      retention.resize(retention.size() + 1);
      retention[i].resize(5);
      std::istringstream stream(str); //型合わせ
      int j = 0;
      while(getline(stream, tmp, '\t')){
        retention[i][j] = tmp;
        j++;
      }
      i++;
    }
    //書き込み
    std::ofstream ofs(filename, std::ios::out);
    if (!ofs)
    {
      std::cerr << "ファイルopen失敗(calculationloadsaveAUC): " << filename << std::endl;
    }
    else
    {   
      for(int i=0; i < Current_end+1; i++){
        for(int j=0; j < 10; j++){
        //MAE, F-measure, AUCの順
        ofs << retention[i*10+j][0] << "\t"
            << retention[i*10+j][1] << "\t"
            << retention[i*10+j][2] << "\t"
            << retention[i*10+j][3] << "\t"
            << std::fixed << std::setprecision(10)
            << Auc_recalculation(i, j, dir) << std::endl;
        }
      }
      
      ofs.close();
    }
  }
  ifs.close();
  return;
}

void Recom::reset(void)
{
  SparseIncompleteData = SparseCorrectData;
  for (int k = 0; k < return_user_number(); k++)
  {
    for (int l = 0; l < return_user_number(); l++)
    {
      Similarity[k][l] = 0.0;
    }
  }
  return;
}

void Recom::reset2()
{
  for (int miss = 0; miss < Missing; miss++)
    Prediction[miss] = 0.0;
  for (int i = 0; i < (int)return_max_value() * 10; i++)
  {
    TP_FN[i] = 0.0;
    FP_TN[i] = 0.0;
  }
  return;
}

void Recom::reset_aveMAE(void)
{
  aveMAE = 0.0;
  return;
}

void Recom::change_median(void)
{
  for (int k = 0; k < return_user_number(); k++)
  {
    for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
    {
      int flag = 0;
      for (int m = 0; m < Missing; m++)
      {
        if ((k == KessonIndex[m][0]) && (ell == SparseIndex[m]))
        {
          flag = 1;
          break;
        }
      }
      if (flag == 0)
      {
        SparseIncompleteData[k].elementIndex(ell) -= return_median();
        // std::cout<<k<<"\t"<<ell<<"\t"<<SparseIncompleteData[k].elementIndex(ell)<<std::endl;
      }
    }
  }
  return;
}

void Recom::change_median_artificiality(void)
{
  for (int k = 0; k < return_user_number(); k++)
  {
    for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
    {
      int flag = 0;
      for (int m = 0; m < Missing; m++)
      {
        if ((k == KessonIndex[m][0]) && (ell == SparseIndex[m]))
        {
          flag = 1;
          break;
        }
      }
      if (flag == 0)
      {
        SparseIncompleteData[k].elementIndex(ell) -= return_median();
        // std::cout<<k<<"\t"<<ell<<"\t"<<SparseIncompleteData[k].elementIndex(ell)<<std::endl;
      }
    }
  }
  return;
}

void Recom::change_aveData(void)
{
  // int a;
  for (int k = 0; k < return_user_number(); k++)
  {
    int count = 0;
    for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
    {
      int flag = 0;
      for (int m = 0; m < Missing; m++)
      {
        if ((k == KessonIndex[m][0]) && (ell == SparseIndex[m]))
        {
          flag = 1;
          break;
        }
      }
      if (flag == 0)
      {
        aveData[k] += SparseIncompleteData[k].elementIndex(ell);
        count++;
      }
    }
    aveData[k] /= count;
    // std::cout<<aveData[k]<<"\t"<<count<<std::endl;

    for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
    {
      int flag = 0;
      for (int m = 0; m < Missing; m++)
      {
        if ((k == KessonIndex[m][0]) && (ell == SparseIndex[m]))
        {
          flag = 1;
          break;
        }
      }
      if (flag == 0)
      {
        SparseIncompleteData[k].elementIndex(ell) -= aveData[k];
      }
    }
    // std::cout<<SparseIncompleteData[k]<<std::endl;
    // std::cin>>a;
  }
  /*
    for(int m=0;m<Missing;m++){
      SparseIncompleteData[KessonIndex[m][0]].elementIndex(SparseIndex[m])=0;
    }
    */
  return;
}

/*

void Recom::change_aveData(void){
  for(int k=0;k<return_user_number();k++){
    for(int ell=0;ell<SparseIncompleteData[k].essencialSize();ell++){
      aveData[k]+=SparseIncompleteData[k].elementIndex(ell);
    }
    aveData[k]/=return_item_number();
    for(int ell=0;ell<SparseIncompleteData[k].essencialSize();ell++){
      SparseIncompleteData[k].elementIndex(ell)-=aveData[k];
    }
  }
  //std::cout<<SparseIncompleteData<<std::endl;
  return;
}
*/

void Recom::revise_missing_values_new(void)
{
  int tmprow, tmpcol;
  for (int m = 0; m < Missing;)
  {
    /****乱数生成****/
    std::mt19937_64 mt;
    mt.seed(SEED);
    std::uniform_int_distribution<>
        randRow(0, return_user_number() - 1);
    //ランダムに行番号生成
    tmprow = randRow(mt);
    std::uniform_int_distribution<>
        randCol(0, SparseCorrectData[tmprow].essencialSize() - 1);
    //ランダムに列番号生成
    tmpcol = randCol(mt);
    //データ行すべて欠損させないように,一行5要素は残す
    int c = 0;
    for (int i = 0; i < SparseIncompleteData[tmprow].essencialSize(); i++)
      if (SparseIncompleteData[tmprow].elementIndex(i) == 0)
        c++;
    //既に欠損していない場合
    if (SparseIncompleteData[tmprow].elementIndex(tmpcol) > 0 && SparseIncompleteData[tmprow].essencialSize() - c > 5)
    {
      //要素を0にする
      SparseIncompleteData[tmprow].elementIndex(tmpcol) = 0;
      //欠損した行番号を保存
      KessonIndex[m][0] = tmprow;
      //欠損した列番号を保存
      KessonIndex[m][1] = SparseIncompleteData[tmprow]
                              .indexIndex(tmpcol);
      //スパースデータの列番号を保存
      SparseIndex[m] = tmpcol;
      m++;
    }
    SEED++;
  }
  return;
}

void Recom::revise_missing_values(void)
{
  int tmprow, tmpcol;
  for (int m = 0; m < Missing;)
  {
    /****乱数生成****/
    std::mt19937_64 mt;
    mt.seed(SEED);
    std::uniform_int_distribution<>
        randRow(0, return_user_number() - 1);
    //ランダムに行番号生成
    tmprow=randRow(mt);
    // tmprow=0; //debug: 欠損箇所をダイレクトに指定
    std::uniform_int_distribution<>
        randCol(0, SparseCorrectData[tmprow].essencialSize() - 1);
    //ランダムに列番号生成
    tmpcol=randCol(mt);
    // tmpcol=3; //debug: 欠損箇所をダイレクトに指定
    //(ここミス201821まで)データ行すべて欠損させないように
    int c = -1;
    for (int i = 0; i < SparseIncompleteData[tmprow].essencialSize(); i++)
      if (SparseIncompleteData[tmprow].elementIndex(i) == 0)
        c++;
    //既に欠損していない場合
    if (SparseIncompleteData[tmprow].elementIndex(tmpcol) > 0 && SparseIncompleteData[tmprow].essencialSize() > c)
    {
      //要素を0にする
      SparseIncompleteData[tmprow].elementIndex(tmpcol) = 0;
      //欠損した行番号を保存
      KessonIndex[m][0] = tmprow;
      //欠損した列番号を保存
      KessonIndex[m][1] = SparseIncompleteData[tmprow]
                              .indexIndex(tmpcol);
      //スパースデータの列番号を保存
      SparseIndex[m] = tmpcol;
      m++;
    }
    SEED++;
  }
  //動作確認用
  // std::cout << "SparseIncompleteData:\n" << SparseIncompleteData << std::endl;

  return;
}

void Recom::mae(std::string text, int method_number, bool mfcl)
{
  double result = 0.0;
  for (int m = 0; m < Missing; m++)
  {
    result += fabs(SparseCorrectData[KessonIndex[m][0]]
                       .elementIndex(SparseIndex[m]) -
                   Prediction[m]);
  }
  if(mfcl){
    resultMAE[method_number][0] = result / (double)Missing;
  } else{
    resultMAE[method_number][CCurrent] = result / (double)Missing;
  }
  std::ofstream ofs(text + "/" + METHOD_NAME_recom + "MAE.txt", std::ios::app);
  ofs << Missing << "\t"
      << SEED << "\t"
      << Current << "\t"
      << CCurrent << "\t"
      << std::fixed << std::setprecision(10)
      << result / (double)Missing << std::endl;
  ofs.close();
  return;
}

void Recom::fmeasure(std::string text, int method_number, bool mfcl)
{
  std::ofstream ofs(text + "/" + METHOD_NAME_recom + "Fmeasure.txt", std::ios::app);
  for (int index = 1; index < (int)return_max_value() * 10; index++)
  {
    double TP = 0.0, FP = 0.0, FN = 0.0, TN = 0.0;
    //閾値の設定
    double siki = (double)index / 10.0;
    for (int m = 0; m < Missing; m++)
    {
      //正解値が閾値以上かつ，予測値が閾値以上場合
      if ((siki <= SparseCorrectData[KessonIndex[m][0]]
                       .elementIndex(SparseIndex[m])) &&
          (siki <= Prediction[m]))
        TP += 1.0;
      //正解値が閾値を下回ったかつ，予測値が閾値上回った場合
      else if ((siki > SparseCorrectData[KessonIndex[m][0]]
                           .elementIndex(SparseIndex[m])) &&
               (siki <= Prediction[m]))
        FP += 1.0;
      //正解値が閾値上回ったかつ，予測値が閾値を下回った場合
      else if ((siki <= SparseCorrectData[KessonIndex[m][0]]
                            .elementIndex(SparseIndex[m])) &&
               (siki > Prediction[m]))
        FN += 1.0;
      //それ以外
      else if ((siki > SparseCorrectData[KessonIndex[m][0]]
                           .elementIndex(SparseIndex[m])) &&
               (siki > Prediction[m]))
        TN += 1.0;
      else
        continue;
    }
    //閾値がF-measureで設定した閾値だった場合
    if (siki == return_threshold())
    {
      double Precision = TP / (TP + FP);
      double Recall = TP / (TP + FN);
      double dummy = (2.0 * Recall * Precision) / (Recall + Precision);
      if (std::isnan(dummy)){
        dummy = 0.0;
      }
      if(mfcl){
        resultFmeasure[method_number][0] = dummy;
      } else {
        resultFmeasure[method_number][CCurrent] = dummy;
      }
      ofs << Missing << "\t"
          << SEED << "\t"
          << current() << "\t"
          << Ccurrent() << "\t"
          << TP << "\t"
          << FP << "\t"
          << FN << "\t"
          << TN << "\t"
          << std::fixed << std::setprecision(10)
          << dummy
          << std::endl;
    }
    /*~2017/12/25
    //0で割る場合，無理やり回避
    if((TP+FN)==0||(FP+TN)==0){
    TP_FN[index]=0.0;
    FP_TN[index]=0.0;
    }
    */
    if (TP + TN == Missing)
    {
      TP_FN[index] = 1.0;
      FP_TN[index] = 1.0;
    }
    // Recall，Falloutの計算
    else
    {
      TP_FN[index] = TP / (TP + FN);
      FP_TN[index] = FP / (FP + TN);
      if ((TP + FN) == 0 || (FP + TN) == 0)
      {
        TP_FN[index] = 1.0;
        FP_TN[index] = 1.0;
      }
      //~2017/12/25
      /*
  if(TP_FN[index]==0||FP_TN[index]==0){
  TP_FN[index]=0;
  FP_TN[index]=0;
  }
      */
    }
  }
  ofs.close();
  return;
}

void Recom::roc(std::string dir)
{
  std::string ROC_STR = dir + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current) + "_" + std::to_string(CCurrent) + "sort.txt";
  // ROCでプロットする点の数
  int max_index = (int)return_max_value() * 10;
  //一旦保存
  Vector False = FP_TN;
  Vector True = TP_FN;
  std::ofstream ofs(ROC_STR, std::ios::app);
  if (!ofs)
    std::cerr << "ファイルオープン失敗(Recom::roc)\n";
  else
  {
    //横軸でソート
    Sort(False, True, max_index);
    for (int i = 0; i < max_index; i++)
      ofs << std::fixed << std::setprecision(10)
          << False[i] << "\t" << True[i] << std::endl;
  }
  ofs.close();
  return;
}

void Recom::Sort(Vector &fal, Vector &tru, int index)
{
  double tmp1, tmp2;
  for (int j = 0; j < index - 1; j++)
  {
    for (int k = j + 1; k < index; k++)
    {
      if (fal[j] > fal[k])
      {
        tmp1 = fal[j];
        tmp2 = tru[j];
        fal[j] = fal[k];
        tru[j] = tru[k];
        fal[k] = tmp1;
        tru[k] = tmp2;
      }
    }
  }
  return;
}

void Recom::auc(std::string dir)
{
  int max = (int)return_max_value() * 10;
  double rocarea = 0.0;
  Vector False = FP_TN;
  Vector True = TP_FN;
  Sort(False, True, max);
  for (int i = 0; i < max - 1; i++)
  {
    if ((False[i] < False[i + 1]))
    {
      double low = False[i + 1] - False[i];
      double height = fabs(True[i + 1] - True[i]);
      double squarearea;
      if(True[i] < True[i + 1]){
        squarearea = low * True[i];
      } else {
        squarearea = low * True[i + 1];
      }
      double triangle = (low * height) / 2.0;
      rocarea += squarearea + triangle;
    }
  }
  std::ofstream ofs(dir + "/" + METHOD_NAME_recom + "AUC.txt", std::ios::app);
  if (!ofs)
  {
    std::cerr << "ファイルopen失敗(auc):trials:" << std::endl;
  }
  else
  {
    ofs << Missing << "\t"
        << SEED << "\t"
        << Current << "\t"
        << CCurrent << "\t"
        << std::fixed << std::setprecision(10)
        << rocarea << std::endl;
  }
  ofs.close();
  return;
}

void Recom::ofs_objective(std::string dir)
{
  std::ofstream ofs(dir + "/object.txt", std::ios::app);
  if (!ofs)
  {
    std::cerr << "ファイルオープン失敗(ofs_objective)\n";
    exit(1);
  }
  ofs << Missing << "\t" << SEED << "\t";
  return;
}

int Recom::min_objective_index(void)
{
  double tmp = DBL_MAX;
  int obje_index = 0;
  for (int i = 0; i < CLUSTERINGTRIALS; i++)
    if (tmp > Obje[i])
    {
      tmp = Obje[i];
      obje_index = i;
    }
  return obje_index;
}

/*
指定が無ければp = 1
p = 0   : GROUPLENS
p = 1   : クラスタリング+GL
それ以外: MF+クラスタリングモード
*/
void Recom::choice_mae_f(std::vector<std::string> dir, int p)
{
  for (int method = 0; method < (int)dir.size(); method++)
  {
    int obje_index = min_objective_index();
    if (p == 0){
        choiceMAE[method][Current] = resultMAE[method][0];
        choiceFmeasure[method][Current] = resultFmeasure[method][0];
        //選ばれたROCをchoiceフォルダに移す
        std::string oldname = dir[method] + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "_" + std::to_string(obje_index) + "sort.txt";
        std::string newname = dir[method] + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "sort.txt";
        Rename(oldname, newname);
    } else if(p == 1){
        choiceMAE[method][Current] = resultMAE[method][obje_index];
        choiceFmeasure[method][Current] = resultFmeasure[method][obje_index];
        bestObj[method][Current] = obje_index;
        //選ばれたROCをchoiceフォルダに移す
        std::string oldname = dir[method] + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "_" + std::to_string(obje_index) + "sort.txt";
        std::string newname = dir[method] + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "sort.txt";
        Rename(oldname, newname);
    } else { //MF+クラスタリング用
        choiceMAE[method][Current] = resultMAE[method][0];
        choiceFmeasure[method][Current] = resultFmeasure[method][0];
        //クラスタリング初期値のナンバリングを消す
        std::string oldname = dir[method] + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "_" + std::to_string(obje_index) + "sort.txt";
        std::string newname = dir[method] + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current + Current_start) + "sort.txt";
        Rename(oldname, newname);
        // std::cout << "Open: " << oldname << std::endl;
    }
  }
  return;
}

void Recom::save_roc_for_artificiality(std::string dir, bool mfcl)
{
  std::string str_roc;
  if(mfcl){
    str_roc = "/ROC/";
  } else {
    str_roc = "/ROC/choice/";
  }
  std::string ROC_STR = dir + str_roc + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(Current) + "sort.txt";
  // ROCでプロットする点の数
  int max_index = (int)return_max_value() * 10;
  //一旦保存
  Vector False = FP_TN;
  Vector True = TP_FN;
  std::ofstream ofs(ROC_STR, std::ios::app);
  if (!ofs)
    std::cerr << "ファイルオープン失敗(save_roc_for_artificiality)\n";
  else
  {
    //横軸でソート
    Sort(False, True, max_index);
    for (int i = 0; i < max_index; i++)
      ofs << std::fixed << std::setprecision(10)
          << False[i] << "\t" << True[i] << std::endl;
  }
  ofs.close();
  return;
}

void Recom::save_mae_f(std::vector<std::string> dir)
{
  for (int method = 0; method < (int)dir.size(); method++)
  {
    choiceMAE[method][Current] = resultMAE[method][0];
    choiceFmeasure[method][Current] = resultFmeasure[method][0];
  }
  return;
}

void Recom::out_mae_f(std::vector<std::string> dir)
{
  for (int method = 0; method < (int)dir.size(); method++)
  {
    std::ofstream ofs(dir[method] + "/averageMaeFmeasure.txt",
                      std::ios::app);
    double sumMAE = 0.0, sumF = 0.0;
    int i = Current_start;
    do {
      sumMAE += choiceMAE[method][i];
      sumF += choiceFmeasure[method][i];
      i++;
    } while (i <= Current_end);
    
    //前回の実験の値を追加
    if(Current_start > 0){
      sumMAE += Averages[0] * (double)Current_start;
      sumF += Averages[1] * (double)Current_start;
    }

    if (!ofs)
      std::cerr << "ファイルopen失敗: out_mae_f\n";

    double denominator = (double)Current_end + 1.0;

    ofs << Missing << "\t" << std::fixed << std::setprecision(10)
        << "\t" << sumMAE / denominator
        << "\t" << sumF / denominator << std::endl;
    ofs.close();
  }
  return;
}

void Recom::precision_summury(std::vector<std::string> dir)
{
  int max = (int)return_max_value() * 10;
  for (int method = 0; method < (int)dir.size(); method++)
  {
    double rocarea = 0.0;
    for(int i = 0; i < (Current_end - Current_start + 1); i++){
      choiceAUC[method][i] = 0.0;
    }
    for (int x = 0; x < (Current_end - Current_start + 1); x++){
      Vector array1(max, 0.0, "all"), array2(max, 0.0, "all");
      std::ifstream ifs(dir[method] + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(x + Current_start) + "sort.txt");
      if (!ifs)
      {
        std::cerr << "ファイルinput失敗(precision_summury):trials:"
                  << x + Current_start << "miss:" << Missing << std::endl;
        std::cout << dir[method] + "/ROC/choice/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(x + Current_start) + "sort.txt" << std::endl;
        break;
      }
      for (int i = 0; i < max; i++)
        ifs >> array1[i] >> array2[i];
      ifs.close();
      for (int i = 0; i < max - 1; i++)
      {
        /*~2017/12/25
          if((array1[i]<array1[i+1])||(array1[i]!=0)||(array2[i]!=0)){
        */
        if ((array1[i] < array1[i + 1]))
        {
          double low = array1[i + 1] - array1[i];
          double height = fabs(array2[i + 1] - array2[i]);
          double squarearea;
          if(array2[i] < array2[i + 1]){
            squarearea = low * array2[i];
          } else {
            squarearea = low * array2[i + 1];
          }
          double triangle = (low * height) / 2.0;
          rocarea += squarearea + triangle;
          choiceAUC[method][x] += squarearea + triangle;
        }
      }
    }

    double sumMAE = 0.0, sumF = 0.0;
    std::ofstream ofs(dir[method] + "/ChoicedMaeAuc.txt", std::ios::app);
    if (!ofs){
      std::cerr << "ファイルopen失敗: choice_mae_f\n";
      exit(1);
    }
    for (int i = 0; i < (Current_end - Current_start + 1); i++){
      sumMAE += choiceMAE[method][i];
      sumF += choiceFmeasure[method][i];
      ofs << std::fixed << std::setprecision(10)
          << choiceMAE[method][i] << "\t"
          << choiceFmeasure[method][i] << "\t"
          << choiceAUC[method][i] << std::endl;
    }
    
    //前回の実験の値を追加
    if(Current_start > 0){
      sumMAE += Averages[0] * (double)Current_start;
      sumF += Averages[1] * (double)Current_start;
      rocarea += Averages[2] * (double)Current_start;
    }
    
    std::ofstream ofs_ave(dir[method] + "/averageMaeFmeasureAuc.txt", std::ios::trunc);
    if (!ofs_ave)
      std::cerr << "ファイルopen失敗: precision_summury\n";
    
    double denominator = (double)Current_end + 1.0;
    
    std::cout << "miss:" << Missing << "\tMAE=" << sumMAE / denominator
              << "\tF-measure=" << sumF / denominator << "\tAUC="
              << rocarea / denominator << std::endl;
    ofs_ave << Missing
            << "\t" << (int)denominator << std::fixed << std::setprecision(10)
            << "\t" << sumMAE / denominator
            << "\t" << sumF / denominator
            << "\t" << rocarea / denominator << std::endl;
    aveMAE += sumMAE / denominator;

    Averages[0] = sumMAE / denominator;
    Averages[1] = sumF / denominator;
    Averages[2] = rocarea / denominator;
  }
  return;
}

void Recom::precision_summury2(std::vector<std::string> dir)
{
  // std::cout << "dir[0]:" << dir[0] << std::endl;
  int max = (int)return_max_value() * 10;
  for (int method = 0; method < (int)dir.size(); method++)
  {
    std::ifstream ifs_mae(dir[method] + "/" + method_name() + "MAE.txt");
    if (!ifs_mae)
    {
      std::cerr << "ファイルinput失敗(precision_summury2):trials:"
                << method_name() << "MAE.txt" << std::endl;
      std::cout << dir[0] << std::endl;
      break;
    }
    std::ifstream ifs_f(dir[method] + "/" + method_name() + "Fmeasure.txt");
    if (!ifs_f)
    {
      std::cerr << "ファイルinput失敗(precision_summury2):trials:"
                << method_name() << "Fmeasure.txt" << std::endl;
      break;
    }
    std::string str, tmp;
    int colIndex = 0;
    //MAE
    while(getline(ifs_mae, str)){
      std::istringstream stream(str);
      for(int j = 0; j < 3; j++){
        getline(stream, tmp, '\t');
      }
      if(colIndex > std::stoi(tmp)){
        std::cout << "precision_summary2: Structure of File does not correct.("
                  << method_name() << "MAE.txt)" << std::endl;
        return;
      } else if(colIndex < std::stoi(tmp)) {
        choiceMAE[method][colIndex] = -1; //error用出力
      } else {
        for(int j = 0; j < 2; j++){
          getline(stream, tmp, '\t');
        }
        // std::cout << "precision_summary2: tmp = " << tmp << std::endl;
        choiceMAE[method][colIndex] = std::stod(tmp);
      }
      colIndex++;
    }
    colIndex = 0;
    //Fmeasure
    while(getline(ifs_mae, str)){
      getline(ifs_f, str);
      std::istringstream stream(str);
      for(int j = 0; j < 3; j++){
        getline(stream, tmp, '\t');
      }
      if(colIndex > std::stoi(tmp)){
        std::cout << "precision_summary2: Structure of File does not correct.("
                  << method_name() << "Fmeasure.txt)" << std::endl;
        return;
      } else if(colIndex < std::stoi(tmp)) {
        choiceFmeasure[method][colIndex] = -1; //error用出力(未使用なのでここは無くても良い)
      } else {
        for(int j = 0; j < 2; j++){
          getline(stream, tmp, '\t');
        }
        choiceFmeasure[method][colIndex] = std::stod(tmp);
      }
      colIndex++;
    }
    ifs_mae.close();
    ifs_f.close();

    double rocarea = 0.0;
    for(int i = 0; i < (Current_end - Current_start + 1); i++){
      choiceAUC[method][i] = 0.0;
    }
    for (int x = 0; x < Current_end + 1; x++){
      Vector array1(max, 0.0, "all"), array2(max, 0.0, "all");
      if(choiceMAE[method][x] != -1){
        std::ifstream ifs(dir[method] + "/ROC/" + METHOD_NAME_recom + "ROC" + std::to_string(Missing) + "_" + std::to_string(x) + "sort.txt");
        if (!ifs)
        {
          std::cerr << "ファイルinput失敗(precision_summury2): trials:"
                    << x << ", miss:" << Missing << std::endl;
          break;
        }
        for (int i = 0; i < max; i++)
          ifs >> array1[i] >> array2[i];
        ifs.close();
        for (int i = 0; i < max - 1; i++)
        {
          /*~2017/12/25
            if((array1[i]<array1[i+1])||(array1[i]!=0)||(array2[i]!=0)){
          */
          if ((array1[i] < array1[i + 1]))
          {
            double low = array1[i + 1] - array1[i];
            double height = fabs(array2[i + 1] - array2[i]);
            double squarearea;
            if(array2[i] < array2[i + 1]){
              squarearea = low * array2[i];
            } else {
              squarearea = low * array2[i + 1];
            }
            double triangle = (low * height) / 2.0;
            rocarea += squarearea + triangle;
            choiceAUC[method][x] += squarearea + triangle;
          }
        }
      }
    }

    double sumMAE = 0.0, sumF = 0.0;
    std::ofstream ofs(dir[method] + "/ChoicedMaeAuc.txt", std::ios::app);
    if (!ofs){
      std::cerr << "ファイルopen失敗: choice_mae_f\n";
      exit(1);
    }
    for (int i = 0; i < (Current_end - Current_start + 1); i++){
      if(choiceMAE[method][i] == -1){
        ofs << "NaN or Limit of Update" << std::endl;
      } else {
      sumMAE += choiceMAE[method][i];
      sumF += choiceFmeasure[method][i];
      ofs << std::fixed << std::setprecision(10)
          << choiceMAE[method][i] << "\t"
          << choiceFmeasure[method][i] << "\t"
          << choiceAUC[method][i] << std::endl;
      }
    }
    
    // //前回の実験の値を追加
    // if(Current_start > 0){
    //   sumMAE += Averages[0] * (double)Current_start;
    //   sumF += Averages[1] * (double)Current_start;
    //   rocarea += Averages[2] * (double)Current_start;
    // }
    
    std::ofstream ofs_ave(dir[method] + "/averageMaeFmeasureAuc.txt", std::ios::trunc);
    if (!ofs_ave)
      std::cerr << "ファイルopen失敗: precision_summury2\n";
    
    double denominator = (double)Current_end + 1.0;
    
    // std::cout << "miss:" << Missing << "\tMAE=" << sumMAE / denominator
    //           << "\tF-measure=" << sumF / denominator << "\tAUC="
    //           << rocarea / denominator << std::endl;
    ofs_ave << Missing
            << "\t" << (int)denominator << std::fixed << std::setprecision(10)
            << "\t" << sumMAE / denominator
            << "\t" << sumF / denominator
            << "\t" << rocarea / denominator << std::endl;
    aveMAE += sumMAE / denominator;

    Averages[0] = sumMAE / denominator;
    Averages[1] = sumF / denominator;
    Averages[2] = rocarea / denominator;
  }
  return;
}

void Recom::total_average_mae(std::vector<std::string> dir)
{
  std::ofstream ofs(dir[0] + "/totalaverageMae.txt", std::ios::app);
  if (!ofs)
    std::cerr << "ファイルopen失敗: total_average_mae\n";
  std::cout << "averageMAE="
            << aveMAE / ((Missing - KESSON_BEGIN) / KIZAMI) << std::endl;
  ofs << aveMAE / ((Missing - KESSON_BEGIN) / KIZAMI) << std::endl;
}

void Recom::revise_prediction(void)
{
  for (int index = 0; index < Missing; index++)
  {
    double RecomItem = 0.0, RecomUSER = 0.0, RecomITEM = 0.0, RecomUserItem = 0.0;
    double numerator = 0.0, denominator = 0.0;
    double numerator1 = 0.0, denominator1 = 0.0;
    double numerator2 = 0.0, denominator2 = 0.0;
    double numerator3 = 0.0, denominator3 = 0.0;
    /******RecomUser********/
    double RecomUser = user_average(KessonIndex[index][0]);
    int index1 = 0, index2 = 0;
    //行クラスタと列クラスタで数が異なる場合も考える
    for (int i = 0; i < Mem.rows(); i++)
      if (Mem[i][KessonIndex[index][0]] == 1.0)
        index1 = i;
    for (int i = 0; i < ItemMem.rows(); i++)
      if (ItemMem[i][KessonIndex[index][1]] == 1.0)
        index2 = i;
    for (int k = 0; k < return_user_number(); k++)
    {
      int user_size =
          SparseIncompleteData[k].essencialSize();
      /******RecomItem********/
      for (int ell = 0; ell < user_size; ell++)
      {
        int user_index = SparseIncompleteData[k].indexIndex(ell);
        if (user_index > KessonIndex[index][1])
          break;
        double user_element = SparseIncompleteData[k].elementIndex(ell);
        if ((user_element > 0) && (user_index == KessonIndex[index][1]))
        {
          numerator1 += user_element;
          denominator1++;
          break;
        }
      }
      /******RecomUSER********/
      if (Mem[index1][k] == 1.0)
      {
        for (int ell = 0; ell < user_size; ell++)
        {
          double user_element = SparseIncompleteData[k].elementIndex(ell);
          if (user_element > 0)
          {
            numerator2 += user_element;
            denominator2++;
          }
        }
      }
      /******RecomITEM********/
      for (int ell = 0; ell < user_size; ell++)
      {
        int user_index = SparseIncompleteData[k].indexIndex(ell);
        if (ItemMem[index2][user_index] == 1.0)
        {
          double user_element = SparseIncompleteData[k].elementIndex(ell);
          if (user_element > 0)
          {
            numerator3 += user_element;
            denominator3++;
          }
        }
      }
      /******RecomUserItem********/
      if (Mem[index1][k] == 1.0)
      {
        for (int ell = 0; ell < user_size; ell++)
        {
          int user_index = SparseIncompleteData[k].indexIndex(ell);
          if (ItemMem[index2][user_index] == 1.0)
          {
            double user_element = SparseIncompleteData[k]
                                      .elementIndex(ell);
            if (user_element > 0)
            {
              numerator += user_element;
              denominator++;
            }
          }
        }
      }
    }
    if (denominator1 > 0)
      RecomItem = numerator1 / denominator1;
    if (denominator2 > 0)
      RecomUSER = numerator2 / denominator2;
    if (denominator3 > 0)
      RecomITEM = numerator3 / denominator3;
    if (denominator > 0)
      RecomUserItem = numerator / denominator;
    Prediction[index] = RecomUser + RecomItem - RecomUSER - RecomITEM + RecomUserItem;
  }
  return;
}

void Recom::computation_w(void)
{
  for (int k = 0; k < return_user_number(); k++)
  {
    double m = 0.0, s = 0.0;
    double ave = user_average(k);
    int size = SparseIncompleteData[k].essencialSize();
    for (int ell = 0; ell < size; ell++)
    {
      double element = SparseIncompleteData[k].elementIndex(ell);
      if (element > 0.0)
      {
        m++;
        s += pow((element - ave), 2.0);
      }
    }
    W[k] = m * sqrt(s / (double)size);
  }
  return;
}

void Recom::revise_prediction2(const Matrix &V)
{
  for (int index = 0; index < Missing; index++)
  {
    int h = 0, a = KessonIndex[index][0];
    double numerator = 0.0, denominator = 0.0;
    for (int i = 0; i < Mem.rows(); i++)
      if (Mem[i][a] == 1.0)
        h = i;
    for (int k = 0; k < return_user_number(); k++)
    {
      double rTv = 0.0;
      for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
        rTv += SparseIncompleteData[k].elementIndex(ell) * V[h][SparseIncompleteData[k].indexIndex(ell)];
      denominator += /*W[k]*/ Mem[h][k] * rTv;
      for (int ell = 0; ell < SparseIncompleteData[k].essencialSize(); ell++)
      {
        if (KessonIndex[index][1] < SparseIncompleteData[k].indexIndex(ell))
          break;
        if (KessonIndex[index][1] == SparseIncompleteData[k].indexIndex(ell))
        {
          numerator += /*W[k]*/ Mem[h][k] * rTv * SparseIncompleteData[k].elementIndex(ell);
          break;
        }
      }
    }
    if (denominator == 0)
      Prediction[index] = 1.0;
    else
      Prediction[index] = numerator / denominator;
  }
  return;
}

void Recom::pearsonsim(void)
{
  for (int user1 = 0; user1 < return_user_number(); user1++)
  {
    /*ユーザ1の非ゼロサイズ*/
    int user1_size =
        SparseIncompleteData[user1]
            .essencialSize();
    for (int user2 = 0; user2 < return_user_number(); user2++)
    {
      double psum = 0.0, sum1 = 0.0, sum2 = 0.0, sum1sq = 0.0, sum2sq = 0.0;
      double hyokasu = 0.0;
      /*ユーザ1がユーザ2である場合*/
      if (user1 == user2)
        Similarity[user1][user2] = 0.0;
      else
      {
        /*ユーザ2の非ゼロサイズ*/
        int user2_size =
            SparseIncompleteData[user2].essencialSize();
        /*ユーザ2のインデックスを0から*/
        int user2_ell = 0;
        for (int ell = 0; ell < user1_size; ell++)
        {
          /*ユーザ2のインデックスが
            ユーザ2の非ゼロサイズを上回ったらbreak ~20180606*/
          if (user2_size < user2_ell)
            break;
          /*ユーザ1の既評価値*/
          double user1_element = SparseIncompleteData[user1]
                                     .elementIndex(ell);
          /*ユーザ1の既評価値が欠損でなければ計算*/
          if (user1_element > 0)
          {
            //ユーザ1のインデックス*/
            int user1_index = SparseIncompleteData[user1]
                                  .indexIndex(ell);
            while (1)
            {
              if (user2_size == user2_ell)
                break;
              /*ユーザ2のインデックス*/
              int user2_index = SparseIncompleteData[user2].indexIndex(user2_ell);
              /*ユーザ2の方が上回ったらbreak*/
              if (user1_index < user2_index)
                break;
              /*ユーザ2の既評価値*/
              double user2_element = SparseIncompleteData[user2].elementIndex(user2_ell);
              /*インデックスが揃った場合とユーザ既評価値が
          欠損されてなければ計算*/
              if ((user1_index == user2_index) && (user2_element > 0))
              {
                hyokasu += 1.0;
                psum += user1_element * user2_element;
                sum1 += user1_element;
                sum2 += user2_element;
                sum1sq += pow(user1_element, 2.0);
                sum2sq += pow(user2_element, 2.0);
                user2_ell++;
                break;
              }
              user2_ell++;
            }
          }
        }
        double numerator = psum - (sum1 * sum2 / hyokasu);
        double denominator = sqrt((sum1sq - pow(sum1, 2.0) / hyokasu) * (sum2sq - pow(sum2, 2.0) / hyokasu));
        if (denominator == 0 || std::isnan(denominator))
          Similarity[user1][user2] = 0.0;
        else
          Similarity[user1][user2] = numerator / denominator;
        // Similarity[user1][user2]=pow(Similarity[user1][user2],sigma);
        // std::cout<< user1<<"\t"<< user2<<"\t"<<Similarity[user1][user2]<<std::endl;
        // std::cout<<Similarity[user1][user2]<<"\t"<<std::flush;
      }
      // std::cout<< user1<<"\t"<< user2<<"\t"<<Similarity[user1][user2]<<std::endl;
    }
  }
  // std::cout << "Similarity:\n" << Similarity << std::endl;
  return;
}

void Recom::pearsonsim_clustering(void)
{
  for (int user1 = 0; user1 < return_user_number(); user1++)
  {
    /*ユーザ1の既評価数*/
    int user1_size =
        SparseIncompleteData[user1]
            .essencialSize();
    // CLUSTERING_PEAR
    int tmpcenternum = 0;
    for (int i = 0; i < Mem.rows(); i++)
      if (Mem[i][user1] == 1.0)
        tmpcenternum = i;
    for (int user2 = 0; user2 < return_user_number(); user2++)
    {
      double psum = 0.0, sum1 = 0.0, sum2 = 0.0, sum1sq = 0.0, sum2sq = 0.0;
      double hyokasu = 0.0;
      /*ユーザ2がユーザ1である，または
  ユーザ1が属すユーザクラスタに属さないユーザであった場合
  ユーザ2とユーザ1の類似度を0にすることで計算させない*/
      if (user1 == user2 || Mem[tmpcenternum][user2] == 0)
        Similarity[user1][user2] = 0.0;
      else
      {
        /*ユーザ2の既評価数*/
        int user2_size =
            SparseIncompleteData[user2].essencialSize();
        /*現在のユーザ2の既評価値インデックス*/
        int user2_ell = 0;
        for (int ell = 0; ell < user1_size; ell++)
        {
          /*ユーザ2の既評価値インデックスがユーザ2の
            既評価数を上回ったらbreak*/
          if (user2_size < user2_ell)
            break;
          /*ユーザ1の現在の既評価値*/
          double user1_element = SparseIncompleteData[user1]
                                     .elementIndex(ell);
          /*ユーザ1の現在の既評価値が欠損されてなければ計算*/
          if (user1_element > 0)
          {
            /*ユーザ1の現在の評価値インデックスのインデックス*/
            int user1_index = SparseIncompleteData[user1]
                                  .indexIndex(ell);
            while (1)
            {
              if (user2_size == user2_ell)
                break;
              /*ユーザ2の現在の評価値インデックスのインデックス*/
              int user2_index = SparseIncompleteData[user2].indexIndex(user2_ell);
              /*ユーザ2の方が上回ったらbreak*/
              if (user1_index < user2_index)
                break;
              /*現在のユーザの既評価値*/
              double user2_element = SparseIncompleteData[user2].elementIndex(user2_ell);
              /*インデックスが揃った場合とユーザ既評価値が
          欠損されてなければ計算*/
              if ((user1_index == user2_index) && (user2_element > 0))
              {
                hyokasu += 1.0;
                psum += user1_element * user2_element;
                sum1 += user1_element;
                sum2 += user2_element;
                sum1sq += pow(user1_element, 2.0);
                sum2sq += pow(user2_element, 2.0);
                user2_ell++;
                break;
              }
              /*現在のユーザの既評価値インデックスインクリメント*/
              user2_ell++;
            }
          }
        }
        double numerator = psum - (sum1 * sum2 / hyokasu);
        double denominator = sqrt((sum1sq - pow(sum1, 2.0) / hyokasu) * (sum2sq - pow(sum2, 2.0) / hyokasu));
        if (denominator == 0 || std::isnan(denominator))
          Similarity[user1][user2] = 0.0;
        else
          Similarity[user1][user2] = numerator / denominator;
      }
    }
  }
  // std::cout << "Similarity:\n" << Similarity << std::endl;
  return;
}

// PCM用類似度計算
void Recom::pearsonsim_for_pcm(const Matrix &Membership_PCM,
                               const Vector &Threshold)
{
  for (int user1 = 0; user1 < return_user_number(); user1++)
  {
    /*ユーザ1の既評価数*/
    int user1_size =
        SparseIncompleteData[user1]
            .essencialSize();
    for (int user2 = 0; user2 < return_user_number(); user2++)
    {
      double psum = 0.0, sum1 = 0.0, sum2 = 0.0, sum1sq = 0.0, sum2sq = 0.0;
      double hyokasu = 0.0;
      /*ユーザ2がユーザ1である，または
  ユーザ1が属すユーザクラスタに属さないユーザであった場合
  ユーザ2とユーザ1の類似度を0にすることで計算させない*/
      if (user1 == user2 || Membership_PCM[user1][user2] < Threshold[user1])
        Similarity[user1][user2] = 0.0;
      else
      {
        /*ユーザ2の既評価数*/
        int user2_size =
            SparseIncompleteData[user2].essencialSize();
        /*現在のユーザ2の既評価値インデックス*/
        int user2_ell = 0;
        for (int ell = 0; ell < user1_size; ell++)
        {
          /*ユーザ2の既評価値インデックスがユーザ2の
            既評価数を上回ったらbreak*/
          if (user2_size < user2_ell)
            break;
          /*ユーザ1の現在の既評価値*/
          double user1_element = SparseIncompleteData[user1]
                                     .elementIndex(ell);
          /*ユーザ1の現在の既評価値が欠損されてなければ計算*/
          if (user1_element > 0)
          {
            /*ユーザ1の現在の評価値インデックスのインデックス*/
            int user1_index = SparseIncompleteData[user1]
                                  .indexIndex(ell);
            while (1)
            {
              if (user2_size == user2_ell)
                break;
              /*ユーザ2の現在の評価値インデックスのインデックス*/
              int user2_index = SparseIncompleteData[user2].indexIndex(user2_ell);
              /*ユーザ2の方が上回ったらbreak*/
              if (user1_index < user2_index)
                break;
              /*現在のユーザの既評価値*/
              double user2_element = SparseIncompleteData[user2].elementIndex(user2_ell);
              /*インデックスが揃った場合とユーザ既評価値が
          欠損されてなければ計算*/
              if ((user1_index == user2_index) && (user2_element > 0))
              {
                hyokasu += 1.0;
                psum += user1_element * user2_element;
                sum1 += user1_element;
                sum2 += user2_element;
                sum1sq += pow(user1_element, 2.0);
                sum2sq += pow(user2_element, 2.0);
                user2_ell++;
                break;
              }
              /*現在のユーザの既評価値インデックスインクリメント*/
              user2_ell++;
            }
          }
        }
        double numerator = psum - (sum1 * sum2 / hyokasu);
        double denominator = sqrt((sum1sq - pow(sum1, 2.0) / hyokasu) * (sum2sq - pow(sum2, 2.0) / hyokasu));
        if (denominator == 0 || std::isnan(denominator))
          Similarity[user1][user2] = 0.0;
        else
          Similarity[user1][user2] = numerator / denominator;
      }
    }
  }
  return;
}

void Recom::pearsonpred1(void)
{
  for (int index = 0; index < Missing; index++)
  {
    double numerator = 0.0, denominator = 0.0;
    int miss_user_index = KessonIndex[index][1];
    for (int i = 0; i < return_user_number(); i++)
    {
      if (Similarity[KessonIndex[index][0]][i] > 0.0)
      {
        int user_size =
            SparseIncompleteData[i].essencialSize();
        for (int ell = 0; ell < user_size; ell++)
        {
          int user_index = SparseIncompleteData[i].indexIndex(ell);
          if (user_index > miss_user_index)
            break;
          double user_element = SparseIncompleteData[i].elementIndex(ell);
          if ((user_element > 0.0) && (user_index == miss_user_index))
          {
            numerator += Similarity[KessonIndex[index][0]][i] * user_element;
            denominator += Similarity[KessonIndex[index][0]][i];
            break;
          }
        }
      }
    }
    if (denominator == 0)
      Prediction[index] = 0.0;
    else
      Prediction[index] = numerator / denominator;
  }
  return;
}

void Recom::pearsonpred2(void)
{
  for (int index = 0; index < Missing; index++)
  {
    double numerator = 0.0, denominator = 0.0;
    int miss_user_index = KessonIndex[index][1];
    for (int i = 0; i < return_user_number(); i++)
    {
      if (Similarity[KessonIndex[index][0]][i] > 0.0)
      {
        int user_size =
            SparseIncompleteData[i].essencialSize();
        for (int ell = 0; ell < user_size; ell++)
        {
          int user_index = SparseIncompleteData[i].indexIndex(ell);
          if (user_index > miss_user_index)
            break;
          double user_element = SparseIncompleteData[i].elementIndex(ell);
          if ((user_element > 0.0) && (user_index == miss_user_index))
          {
            numerator += Similarity[KessonIndex[index][0]][i] * (user_element - user_average(i));
            denominator += Similarity[KessonIndex[index][0]][i];
            break;
          }
        }
      }
    }
    if (denominator == 0)
      Prediction[index] = user_average(KessonIndex[index][0]);
    else
    {
      Prediction[index] = user_average(KessonIndex[index][0]) + numerator / denominator;
    }
    if (std::isnan(Prediction[index]))
      Prediction[index] = 0.0;
  }
  return;
}

void Recom::pearsonpred2_after_clustering(void)
{
  for (int index = 0; index < Missing; index++)
  {
    double numerator = 0.0, denominator = 0.0;
    int miss_user_index = KessonIndex[index][1];
    int tmpcenternum = 0;
    for (int i = 0; i < Mem.rows(); i++)
      if (Mem[i][KessonIndex[index][0]] == 1.0)
        tmpcenternum = i;
    for (int i = 0; i < return_user_number(); i++)
    {
      if (Similarity[KessonIndex[index][0]][i] > 0.0 &&
          Mem[tmpcenternum][i] == 1.0)
      {
        int user_size =
            SparseIncompleteData[i].essencialSize();
        for (int ell = 0; ell < user_size; ell++)
        {
          int user_index = SparseIncompleteData[i].indexIndex(ell);
          if (user_index > miss_user_index)
            break;
          double user_element = SparseIncompleteData[i].elementIndex(ell);
          if ((user_element > 0.0) && (user_index == miss_user_index))
          {
            numerator += Similarity[KessonIndex[index][0]][i] * (user_element - user_average(i));
            denominator += Similarity[KessonIndex[index][0]][i];
            break;
          }
        }
      }
    }
    if (denominator == 0)
      Prediction[index] = user_average(KessonIndex[index][0]);
    else
    {
      Prediction[index] = user_average(KessonIndex[index][0]) + numerator / denominator;
    }
    if (std::isnan(Prediction[index]))
    {
      Prediction[index] = 0.0;
    }

    // std::cout <<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout <<index<<"\t"<<KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  //動作確認用
  // std::cout << "Prediction:\n" << Prediction << std::endl;
  return;
}

int Recom::mf_pred(std::string dir, double K_percent, double beta, double alpha, int steps) //K以外は初期値有なので指定無しでも可
{
  int K;
  if(return_user_number() > return_item_number()){
    K = std::round(return_item_number() * K_percent / 100);
  } else {
    K = std::round(return_user_number() * K_percent / 100);
  }
  if(steps < 50){
    std::cerr << "MF: \"step\" should be 50 or more.";
    return 1;
  }
  Matrix P(return_user_number(), K), Q(return_item_number(), K);
  double best_error = DBL_MAX;
  int NaNcount = 0;
  int trialLimit = CLUSTERINGTRIALS;
  int mf_seed = 0;
  double qsum,psum;
  // trialLimit = 1; //debug
  for(int rand_mf_trial = 0; rand_mf_trial < trialLimit; rand_mf_trial++){
    std::cout << "MF: initial setting " << rand_mf_trial << std::endl;
    //P, Qの初期値を乱数で決定
    std::mt19937_64 mt;
    qsum = 0.0;
    psum = 0.0;
    for(int k_i = 0; k_i < K; k_i++){
      for(int i = 0; i < P.rows(); i++){
        mt.seed(mf_seed);
        std::uniform_real_distribution<>
            rand_p(0.0, 1.0);
        //ランダムに値生成
        P[i][k_i] = rand_p(mt);
        mf_seed++;
        psum += P[i][k_i];
      }
      for(int j = 0; j < Q.rows(); j++){
        mt.seed(mf_seed);
        std::uniform_real_distribution<>
            rand_q(0.0, 1.0);
        //ランダムに値生成
        Q[j][k_i] = rand_q(mt);
        mf_seed++;
        qsum += Q[j][k_i];
      }
      //std::cout << "P:\n" << P << "\nQ:\n" << Q << std::endl;
    }
    std::cout << "P:\n" << psum << "\nQ:\n" << qsum << std::endl;
    //近似式は R = P^T * Q (P:k×m行列, Q:k×n行列) だが，
    //プログラムの都合上 P:m×k行列, Q:n×k行列 と定義．

    double error = 0.0;
    //double prev_error = DBL_MAX; \\debug
    Matrix prev_P(P.rows(), P.cols(), 0.0);
    Matrix prev_Q(Q.rows(), Q.cols(), 0.0);
    bool ParameterNaN = false;
    for(int step = 0; step < steps; step++){
      for(int i = 0; i < P.rows(); i++){
        for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
          if(SparseIncompleteData[i].elementIndex(j) != 0){
            // e = r - p * q
            double err = SparseIncompleteData[i].elementIndex(j)
                          - (P[i] * Q[SparseIncompleteData[i].indexIndex(j)]);
            //更新式で更新
            for(int k_i = 0; k_i < K; k_i++){
              P[i][k_i]
                += alpha * (2 * err * Q[SparseIncompleteData[i].indexIndex(j)][k_i] - beta * P[i][k_i]);
              Q[SparseIncompleteData[i].indexIndex(j)][k_i]
                += alpha * (2 * err * P[i][k_i] - beta * Q[SparseIncompleteData[i].indexIndex(j)][k_i]);
              if(!isfinite(P[i][k_i])){
                std::cerr << "P[" << i << "][" << k_i << "] is not finite. step = " << step
                          << ", err = " << err 
                          << ", error = " << error 
                          << ", K = " << K 
                          << ", beta = " << beta 
                          << ", alpha = " << alpha 
                          << std::endl;
                ParameterNaN = true;
                break;
              }
              if(!isfinite(Q[SparseIncompleteData[i].indexIndex(j)][k_i])){
                std::cerr << "Q[" << j << "][" << k_i << "] is not finite. step = " << step
                          << ", err = " << err
                          << ", error = " << error 
                          << ", K = " << K 
                          << ", beta = " << beta 
                          << ", alpha = " << alpha 
                          << std::endl;
              //std::cout << "\nQ:\n" << Q << std::endl;
                ParameterNaN = true;
                break;
              }
            }
            if(ParameterNaN)
              break;
          }
          if(ParameterNaN)
            break;
        }
        if(ParameterNaN)
          break;
      }
      if(ParameterNaN)
        break;
      //目的関数値計算
      error = 0.0;
      double P_L2Norm = 0.0, Q_L2Norm = 0.0;
      for(int i = 0; i < P.rows(); i++){
        for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
          if(SparseIncompleteData[i].elementIndex(j) != 0){
          error += pow(
                    SparseIncompleteData[i].elementIndex(j)
                    - (P[i] * Q[SparseIncompleteData[i].indexIndex(j)])
                    , 2);
          }
        }
        P_L2Norm += P[i] * P[i];
      }
      for(int j = 0; j < Q.rows(); j++){
        Q_L2Norm += Q[j] * Q[j];
      }
      error += (beta/2.0) * (P_L2Norm + Q_L2Norm);

      //収束判定
      //if(fabs(prev_error - error) < 1.0E-5){
      //if(prev_error < error){
      double diff = max_norm(prev_P - P) + max_norm(prev_Q - Q);
      //diff出力
      // std::ofstream ofs_diff(dir + "/diff/diff" + std::to_string(rand_mf_trial) + ".txt", std::ios::app);
      // ofs_diff << step << "\t"
      //           << std::fixed << std::setprecision(10) << diff << std::endl;
      // ofs_diff.close();
      //収束条件
      // if(false){ //diff出力用
      if(diff < 0.07 && step >= 50){
        // std::cout << "error = " << error << ", step = " << step << std::endl;
        break;
      }
      prev_P = P;
      prev_Q = Q;
      if(step == steps - 1){
          std::cout<< "step = " << step << ", error = " << error 
                    << ", K: " << K_percent << "%, beta = " << beta
                    << ", alpha = " << alpha << std::endl;
        ParameterNaN = true;
      }
      //prev_error = error;
    }
    if(ParameterNaN){
      NaNcount++;
      //初期値全部{NaN出た or step上限回更新して収束しなかった} => 1を返して終了
      if(NaNcount == trialLimit){
        return 1;
      }
    } else if(error < best_error){
      best_error = error;
      //計算済みのP, Qから評価値予測を実行
      for(int index = 0; index < Missing; index++){
        //欠損箇所だけ計算
        Prediction[index] = P[KessonIndex[index][0]] * Q[KessonIndex[index][1]];
      }
    }
  } //初期値ループ
  //debug
  // int count_pred = 0;
  // for(int index = 0; index < Missing; index++){
  //   if(Prediction[index] > 10.0){
  //     count_pred++;
  //   }
  // }
  // std::cout << "Prediction over 10: " << count_pred << std::endl;
  return 0;
}

int Recom::mf_pred_after_clustering(std::string dir, double K_percent, double beta, double alpha, int steps) //K以外は初期値有なので指定無しでも可
{
  for(int cluster_index = 0; cluster_index < Mem.rows(); cluster_index++){
    //クラスタに属するユーザ数と，それらが評価したアイテム数
    int cluster_user_num = 0, cluster_item_num = 0;
    //クラスタに属するユーザが評価したアイテムかどうか
    bool isItemEvaluated[return_item_number()];
    //元データのインデックス と MFのPとQ の紐付け用Map変数
    //KEY:元データのインデックス, VALUE:MFのPとQ内のインデックス
    std::map<int, int> UserIndexMap, ItemIndexMap;
    for(int i = 0; i < return_item_number(); i++){
      isItemEvaluated[i] = false;
    }
    for(int i = 0; i < return_user_number(); i++){
      if(Mem[cluster_index][i] == 1.0){ //ユーザiがクラスタに属するなら...
        UserIndexMap.insert(std::make_pair(i, cluster_user_num));
        cluster_user_num++;
        for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
          isItemEvaluated[SparseIncompleteData[i].indexIndex(j)] = true;
        }
      }
    }
    for(int i = 0; i < return_item_number(); i++){
      if(isItemEvaluated[i]){
        ItemIndexMap.insert(std::make_pair(i, cluster_item_num));
        cluster_item_num++;
      }
    }
    if(cluster_user_num == 0 || cluster_item_num == 0){
      continue;
    }

    //ここからMF
    int K;
    if(cluster_user_num > cluster_item_num){
      K = std::round(cluster_item_num * K_percent / 100);
    } else {
      K = std::round(cluster_user_num * K_percent / 100);
    }
    if(K == 0){
      K = 1;
    }
    if(steps < 50){
      std::cerr << "MF: \"step\" should be 50 or more.";
      return 1;
    }
    Matrix P(cluster_user_num, K), Q(cluster_item_num, K);
    double best_error = DBL_MAX;
    int NaNcount = 0;
    int trialLimit = CLUSTERINGTRIALS;
    int mf_seed = 0;
    // trialLimit = 1; //debug
    for(int rand_mf_trial = 0; rand_mf_trial < trialLimit; rand_mf_trial++){
      // std::cout << "MF: initial setting " << rand_mf_trial << std::endl;
      //P, Qの初期値を乱数で決定
      std::mt19937_64 mt;
      for(int k_i = 0; k_i < K; k_i++){
        for(int i = 0; i < P.rows(); i++){
          mt.seed(mf_seed);
          std::uniform_real_distribution<>
              rand_p(0.0, 1.0);
          //ランダムに値生成
          P[i][k_i] = rand_p(mt);
          mf_seed++;
        }
        for(int j = 0; j < Q.rows(); j++){
          mt.seed(mf_seed);
          std::uniform_real_distribution<>
              rand_q(0.0, 1.0);
          //ランダムに値生成
          Q[j][k_i] = rand_q(mt);
          mf_seed++;
        }
        //std::cout << "P:\n" << P << "\nQ:\n" << Q << std::endl;
      }
      //近似式は R = P^T * Q (P:k×m行列, Q:k×n行列) だが，
      //プログラムの都合上 P:m×k行列, Q:n×k行列 と定義．

      double error = 0.0;
      //double prev_error = DBL_MAX;
      Matrix prev_P(P.rows(), P.cols(), 0.0);
      Matrix prev_Q(Q.rows(), Q.cols(), 0.0);
      bool ParameterNaN = false;
      for(int step = 0; step < steps; step++){
        int mf_user_index = -1, mf_item_index = -1;
        for(int i = 0; i < return_user_number(); i++){
          if(Mem[cluster_index][i] == 1.0){
            mf_user_index++;
            for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
              if(isItemEvaluated[SparseIncompleteData[i].indexIndex(j)]){
                mf_item_index = ItemIndexMap.at(SparseIncompleteData[i].indexIndex(j));
                if(SparseIncompleteData[i].elementIndex(j) != 0){
                  // e = r - p * q
                  double err = SparseIncompleteData[i].elementIndex(j)
                                - (P[mf_user_index] * Q[mf_item_index]);
                  //更新式で更新
                  for(int k_i = 0; k_i < K; k_i++){
                    P[mf_user_index][k_i]
                      += alpha * (2 * err * Q[mf_item_index][k_i] - beta * P[mf_user_index][k_i]);
                    Q[mf_item_index][k_i]
                      += alpha * (2 * err * P[mf_user_index][k_i] - beta * Q[mf_item_index][k_i]);
                    if(!isfinite(P[mf_user_index][k_i])){
                      std::cerr << "P[" << mf_user_index << "][" << k_i << "] is not finite. step = " << step
                                << ", err = " << err 
                                << ", error = " << error 
                                << ", K = " << K 
                                << ", beta = " << beta 
                                << ", alpha = " << alpha 
                                << std::endl;
                      ParameterNaN = true;
                      break;
                    }
                    if(!isfinite(Q[mf_item_index][k_i])){
                      std::cerr << "Q[" << mf_item_index << "][" << k_i << "] is not finite. step = " << step
                                << ", err = " << err
                                << ", error = " << error 
                                << ", K = " << K 
                                << ", beta = " << beta 
                                << ", alpha = " << alpha 
                                << std::endl;
                    //std::cout << "\nQ:\n" << Q << std::endl;
                      ParameterNaN = true;
                      break;
                    }
                  }
                  if(ParameterNaN)
                    break;
                }
              }
              if(ParameterNaN)
                break;
            }
            mf_item_index = -1;
            if(ParameterNaN)
              break;
          }//if_cluster
        }
        mf_user_index = -1;
        if(ParameterNaN)
          break;
        //目的関数値計算
        error = 0.0;
        double P_L2Norm = 0.0, Q_L2Norm = 0.0;
        for(int i = 0; i < return_user_number(); i++){
          if(Mem[cluster_index][i] == 1.0){
            mf_user_index++;
            for(int j = 0; j < SparseIncompleteData[i].essencialSize(); j++){
              if(isItemEvaluated[SparseIncompleteData[i].indexIndex(j)]){
                mf_item_index = ItemIndexMap.at(SparseIncompleteData[i].indexIndex(j));
                if(SparseIncompleteData[i].elementIndex(j) != 0){
                  error += pow(
                            SparseIncompleteData[i].elementIndex(j)
                            - (P[mf_user_index] * Q[mf_item_index])
                            , 2);
                }
              }
            }
            mf_item_index = -1;
            P_L2Norm += P[mf_user_index] * P[mf_user_index];
          }
        }
        mf_user_index = -1;
        for(int j = 0; j < Q.rows(); j++){
          Q_L2Norm += Q[j] * Q[j];
        }
        error += (beta/2.0) * (P_L2Norm + Q_L2Norm);

        //if(fabs(prev_error - error) < 1.0E-5){ //収束判定
        //if(prev_error < error){
        double diff = max_norm(prev_P - P) + max_norm(prev_Q - Q);
        //diff出力
        // std::ofstream ofs_diff(dir + "/diff/diff" + std::to_string(rand_mf_trial) + ".txt", std::ios::app);
        // ofs_diff << step << "\t"
        //           << std::fixed << std::setprecision(10) << diff << std::endl;
        // ofs_diff.close();
        //収束条件
      // if(false){ //diff出力用
        if(diff < 0.07 && step >= 50){
          // std::cout << "error = " << error << ", step = " << step << std::endl;
          break;
        }
        prev_P = P;
        prev_Q = Q;
        if(step == steps - 1){
          std::cout<< "step = " << step << ", error = " << error 
                    << ", K: " << K_percent << "%, beta = " << beta
                    << ", alpha = " << alpha << std::endl;
          ParameterNaN = true;
        }
        //prev_error = error;
      }
      if(ParameterNaN){
        NaNcount++;
        //初期値全部{NaN出た or step上限回更新して収束しなかった} => 1を返して終了
        if(NaNcount == trialLimit){
          return 1;
        }
      } else if(error < best_error){
        best_error = error;
        //計算済みのP, Qから評価値予測を実行
        for(int index = 0; index < Missing; index++){
          //欠損箇所だけかつ，今回のクラスタの分だけ計算
          if(Mem[cluster_index][KessonIndex[index][0]] == 1.0){
            if(isItemEvaluated[(int)KessonIndex[index][1]]){
              Prediction[index]
              = P[UserIndexMap.at(KessonIndex[index][0])] * Q[ItemIndexMap.at(KessonIndex[index][1])];
            }
          }
        }
      }
    } //初期値ループ
  } //cluster_index
  // std::cout << "Prediction: " << Prediction << std::endl;
  return 0;
}

int Recom::returnBestClusteringInitial(std::string method, int kesson, std::vector<double> parameter, int centerNum, int current){
  std::vector<std::string> path = MkdirFCS(method, kesson);
  path = Mkdir(parameter, centerNum, path); 
  path[0] += "/ROC/" + method + "ROC" + std::to_string(kesson) + "_" + std::to_string(current) + "_";
  // std::cout << path[0] << std::endl;
  int missingCount = 0, missingIndex = -1;
  for(int i = 0; i < CLUSTERINGTRIALS; i++){
    if( !std::filesystem::exists(path[0] + std::to_string(i) + "sort.txt") ){
      missingIndex = i;
      missingCount++;
    }
  }
  if(missingCount != 1){
    missingIndex = -1;
  }
  return missingIndex;
}

/*
void Recom::Ubukata(int clusters_number){
  for(int index=0;index<Missing;index++){
    Prediction[index]=0;
    //std::cout << KessonIndex[index][0]<<"\t"<<KessonIndex[index][1] << std::endl;
    for(int i=0;i<clusters_number;i++){
      Prediction[index]+=Mem[i][KessonIndex[index][0]]*ItemMem[i][KessonIndex[index][1]];
      //std::cout << Membership[i][KessonIndex[index][0]]<<"\t" << Centers[i][KessonIndex[index][1]]<<"\t" << Membership[i][KessonIndex[index][0]]*Centers[i][KessonIndex[index][1]]<< "\t" << Prediction[index]<< std::endl;
      //std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    }
    std::cout <<index<<"\t"<< KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    //std::cout << Prediction[index] << std::endl;


  }
  //std::cout << std::endl;
  return;
}
*/

void Recom::Ubukata(int clusters_number)
{
  for (int index = 0; index < Missing; index++)
  {
    Vector prediction(return_item_number(), 0.0, "all");
    // std::cout << KessonIndex[index][0]<<"\t"<<KessonIndex[index][1] << std::endl;
    for (int i = 0; i < clusters_number; i++)
    {
      prediction += Mem[i][KessonIndex[index][0]] * ItemMem[i];
      // if(index==9973)
      // std::cout << Mem[i][KessonIndex[index][0]]<<"\t" << ItemMem[i][KessonIndex[index][1]]<<"\t" <<Mem[i][KessonIndex[index][0]]*ItemMem[i][KessonIndex[index][1]]<< "\t" <<prediction[KessonIndex[index][1]]<< std::endl;
      // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    }
    Prediction[index] = prediction[KessonIndex[index][1]];
    // std::cout <<index<<"\t"<< KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout << Prediction[index] << std::endl;
  }
  // std::cout << std::endl;
  return;
}

void Recom::UbukataForSphericalData(int clusters_number)
{
  // std::cout<<"UbukataForSphericalData"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    Vector prediction(return_item_number(), 0.0, "all");
    // std::cout << KessonIndex[index][0]<<"\t"<<KessonIndex[index][1] << std::endl;
    for (int i = 0; i < clusters_number; i++)
    {
      prediction += Mem[i][KessonIndex[index][0]] * ItemMem[i];
      // std::cout << Membership[i][KessonIndex[index][0]]<<"\t" << Centers[i][KessonIndex[index][1]]<<"\t" << Membership[i][KessonIndex[index][0]]*Centers[i][KessonIndex[index][1]]<< "\t" << Prediction[index]<< std::endl;
      // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    }
    Prediction[index] = prediction[KessonIndex[index][1]] / squared_norm(prediction);
    // std::cout <<index<<"\t"<< KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout << Prediction[index] << std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  // std::cout << std::endl;
  return;
}

void Recom::RestoreForSphericalData(const Vector &Sum)
{
  // std::cout<<"RestoreForSphericalData"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    Prediction[index] *= Sum[KessonIndex[index][0]];
    // std::cout <<KessonIndex[index][0]<<"\t"<<Sum[KessonIndex[index][0]]  << std::endl;
    // std::cout << Prediction[index] << std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  return;
}

void Recom::restore_aveData(void)
{
  // std::cout<<"restore_median"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    // Prediction[index]+=return_median();
    Prediction[index] = Prediction[index] + aveData[KessonIndex[index][0]];
    // std::cout <<KessonIndex[index][0]<<"\t"<<Sum[KessonIndex[index][0]]  << std::endl;
    // std::cout << Prediction[index] << std::endl;
    // if(index==9973)
    // std::cout <<index<<"\t"<<KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<<"\t"<<aveData[KessonIndex[index][0]]<< std::endl;
  }
  return;
}

void Recom::restore_median(double correction)
{
  // std::cout<<"restore_median"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    // Prediction[index]+=return_median();
    Prediction[index] = correction * Prediction[index] + return_median();
    // std::cout <<KessonIndex[index][0]<<"\t"<<Sum[KessonIndex[index][0]]  << std::endl;
    // std::cout << Prediction[index] << std::endl;
    // if(index==9973)
    // std::cout <<index<<"\t"<<KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  return;
}

void Recom::RestoreForMMMData(const Vector &Sum)
{
  for (int index = 0; index < Missing; index++)
  {
    // std::cout <<Prediction[index]<<"\t"<<Sum[KessonIndex[index][0]]  << std::endl;
    Prediction[index] *= Sum[KessonIndex[index][0]];
    // std::cout <<KessonIndex[index][0]<<"\t"<<Sum[KessonIndex[index][0]]  << std::endl;
    // std::cout << Prediction[index] << std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  return;
}

void Recom::Ubukata_CentersSim(int clusters_number)
{
  Matrix Similarity_Centers(clusters_number, clusters_number);
  for (int user1 = 0; user1 < clusters_number; user1++)
  {
    for (int user2 = 0; user2 < clusters_number; user2++)
    {
      double psum = 0.0, sum1 = 0.0, sum2 = 0.0, sum1sq = 0.0, sum2sq = 0.0;
      double hyokasu = 0.0;
      /*ユーザ1がユーザ2である場合*/
      if (user1 == user2)
      {
        Similarity_Centers[user1][user2] = 1.0;
        std::cout << "check:1" << std::endl;
      }
      else
      {
        std::cout << "check:2" << std::endl;
        for (int ell = 0; ell < clusters_number; ell++)
        {
          /*ユーザ1の既評価値*/
          double user1_element = ItemMem[user1][ell];
          /*ユーザ2の既評価値*/
          double user2_element = ItemMem[user2][ell];
          hyokasu += 1.0;
          psum += user1_element * user2_element;
          sum1 += user1_element;
          sum2 += user2_element;
          sum1sq += pow(user1_element, 2.0);
          sum2sq += pow(user2_element, 2.0);
        }
        double numerator = psum - (sum1 * sum2 / hyokasu);
        double denominator = sqrt((sum1sq - pow(sum1, 2.0) / hyokasu) * (sum2sq - pow(sum2, 2.0) / hyokasu));
        if (denominator == 0 || std::isnan(denominator))
          Similarity_Centers[user1][user2] = 0.0;
        else
          Similarity_Centers[user1][user2] = numerator / denominator;
        // Similarity[user1][user2]=pow(Similarity[user1][user2],sigma);
        // std::cout<< user1<<"\t"<< user2<<"\t"<<Similarity[user1][user2]<<std::endl;
        // std::cout<<Similarity[user1][user2]<<"\t"<<std::flush;
      }
      std::cout << user1 << "\t" << user2 << "\t" << Similarity_Centers[user1][user2] << std::endl;
    }
  }

  std::cout << Similarity_Centers << std::endl;

  // std::cout<<"UbukataForSphericalData"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    int TopCluster = 0;
    for (int c = 1; c < clusters_number; c++)
    {
      if (Mem[c][KessonIndex[index][0]] > Mem[TopCluster][KessonIndex[index][0]])
      {
        TopCluster = c;
      }
    }
    Vector prediction(return_item_number(), 0.0, "all");
    double MemSum = 0;
    // std::cout << KessonIndex[index][0]<<"\t"<<KessonIndex[index][1] << std::endl;
    for (int i = 0; i < clusters_number; i++)
    {
      if (Similarity_Centers[TopCluster][i] > 0)
      {
        prediction += Mem[i][KessonIndex[index][0]] * ItemMem[i];
        MemSum += Mem[i][KessonIndex[index][0]];
        // std::cout <<index<<"\t"<<TopCluster<<"\t"<<i<< std::endl;
      }
      // std::cout << Membership[i][KessonIndex[index][0]]<<"\t" << Centers[i][KessonIndex[index][1]]<<"\t" << Membership[i][KessonIndex[index][0]]*Centers[i][KessonIndex[index][1]]<< "\t" << Prediction[index]<< std::endl;
      // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    }
    Prediction[index] = prediction[KessonIndex[index][1]] / (MemSum);
    // std::cout <<index<<"\t"<< KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout << Prediction[index] << std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  // std::cout << std::endl;
  return;
}

void Recom::UbukataForSphericalData_CentersSim(int clusters_number)
{
  Matrix Similarity_Centers(clusters_number, clusters_number);
  for (int user1 = 0; user1 < clusters_number; user1++)
  {
    for (int user2 = 0; user2 < clusters_number; user2++)
    {
      double psum = 0.0, sum1 = 0.0, sum2 = 0.0, sum1sq = 0.0, sum2sq = 0.0;
      double hyokasu = 0.0;
      /*ユーザ1がユーザ2である場合*/
      if (user1 == user2)
      {
        Similarity_Centers[user1][user2] = 1.0;
        // std::cout<<"check:1"<<std::endl;
      }
      else
      {
        // std::cout<<"check:2"<<std::endl;
        for (int ell = 0; ell < clusters_number; ell++)
        {
          /*ユーザ1の既評価値*/
          double user1_element = ItemMem[user1][ell];
          /*ユーザ2の既評価値*/
          double user2_element = ItemMem[user2][ell];
          hyokasu += 1.0;
          psum += user1_element * user2_element;
          sum1 += user1_element;
          sum2 += user2_element;
          sum1sq += pow(user1_element, 2.0);
          sum2sq += pow(user2_element, 2.0);
        }
        double numerator = psum - (sum1 * sum2 / hyokasu);
        double denominator = sqrt((sum1sq - pow(sum1, 2.0) / hyokasu) * (sum2sq - pow(sum2, 2.0) / hyokasu));
        if (denominator == 0 || std::isnan(denominator))
          Similarity_Centers[user1][user2] = 0.0;
        else
          Similarity_Centers[user1][user2] = numerator / denominator;
        // Similarity[user1][user2]=pow(Similarity[user1][user2],sigma);
        // std::cout<< user1<<"\t"<< user2<<"\t"<<Similarity[user1][user2]<<std::endl;
        // std::cout<<Similarity[user1][user2]<<"\t"<<std::flush;
      }
      // std::cout<< user1<<"\t"<< user2<<"\t"<<Similarity_Centers[user1][user2]<<std::endl;
    }
  }

  // std::cout<<Similarity_Centers<<std::endl;

  // std::cout<<"UbukataForSphericalData"<<std::endl;
  for (int index = 0; index < Missing; index++)
  {
    int TopCluster = 0;
    for (int c = 1; c < clusters_number; c++)
    {
      if (Mem[c][KessonIndex[index][0]] > Mem[TopCluster][KessonIndex[index][0]])
      {
        TopCluster = c;
      }
    }
    Vector prediction(return_item_number(), 0.0, "all");
    double MemSum = 0;
    // std::cout << KessonIndex[index][0]<<"\t"<<KessonIndex[index][1] << std::endl;
    for (int i = 0; i < clusters_number; i++)
    {
      if (Similarity_Centers[TopCluster][i] > 0)
      {
        prediction += Mem[i][KessonIndex[index][0]] * ItemMem[i];
        MemSum += Mem[i][KessonIndex[index][0]];
        // std::cout <<index<<"\t"<<TopCluster<<"\t"<<i<< std::endl;
      }
      // std::cout << Membership[i][KessonIndex[index][0]]<<"\t" << Centers[i][KessonIndex[index][1]]<<"\t" << Membership[i][KessonIndex[index][0]]*Centers[i][KessonIndex[index][1]]<< "\t" << Prediction[index]<< std::endl;
      // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    }
    Prediction[index] = prediction[KessonIndex[index][1]] / (MemSum * squared_norm(prediction));
    // std::cout <<index<<"\t"<< KessonIndex[index][0]<<":"<<KessonIndex[index][1]<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
    // std::cout << Prediction[index] << std::endl;
    // std::cout <<index<<"\t"<<SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])<< "\t" <<Prediction[index]<< std::endl;
  }
  // std::cout << std::endl;
  return;
}

void Recom::save_prediction(std::string dir)
{
  std::string prediction_STR = dir + "/prediction/" + METHOD_NAME_recom + "prediction" + std::to_string(Missing) + "_" + std::to_string(Current) + "_" + std::to_string(CCurrent) + "sort.txt";
  std::ofstream ofs(prediction_STR, std::ios::app);
  if (!ofs)
    std::cerr << "ファイルオープン失敗(save_predicition)\n";
  else
  {
    for (int index = 0; index < Missing; index++)
    {
      ofs << index << "\t" << KessonIndex[index][0] << "\t" << KessonIndex[index][1] << "\t"
          << SparseCorrectData[KessonIndex[index][0]].elementIndex(SparseIndex[index])
          << "\t" << Prediction[index] << std::endl;
    }
  }
  ofs.close();
  return;
}

void Recom::filtering_similarities(void)
{
  for (int user1 = 0; user1 < return_user_number(); user1++)
  {
    int tmpcenternum = 0;
    for (int i = 0; i < Mem.rows(); i++)
      if (Mem[i][user1] == 1.0)
        tmpcenternum = i;
    for (int user2 = 0; user2 < return_user_number(); user2++)
      if (Mem[tmpcenternum][user2] == 0)
        Similarity[user1][user2] = 0.0;
  }
  return;
}

double Recom::user_average(int index)
{
  double result = 0.0;
  int hyokazu = 0;
  for (int i = 0; i < SparseIncompleteData[index].essencialSize(); i++)
  {
    if (SparseIncompleteData[index].elementIndex(i) > 0.0)
    {
      result += SparseIncompleteData[index].elementIndex(i);
      hyokazu++;
    }
  }
  return result / hyokazu;
}

SparseMatrix Recom::sparsecorrectdata(void) const
{
  return SparseCorrectData;
}

SparseVector &Recom::sparsecorrectdata(int index)
{
  return SparseCorrectData[index];
}

SparseMatrix Recom::sparseincompletedata(void) const
{
  return SparseIncompleteData;
}

SparseVector &Recom::sparseincompletedata(const int &index)
{
  return SparseIncompleteData[index];
}

void Recom::crisp(const Matrix &Membership)
{
  for (int k = 0; k < return_user_number(); k++)
  {
    for (int i = 0; i < Membership.rows(); i++)
      Mem[i][k] = 0.0;
    double max = -DBL_MAX;
    int max_index = -1;
    for (int i = 0; i < Membership.rows(); i++)
    {
      if (Membership[i][k] > max)
      {
        max = Membership[i][k];
        max_index = i;
      }
    }
    Mem[max_index][k] = 1.0;
  }
  // std::cout<<"membership:\n"<<Mem<<std::endl;
  return;
}

void Recom::copy_mem_cen(const Matrix &Membership,
                         const Matrix &ItemMembership)
{
  for (int k = 0; k < return_user_number(); k++)
  {
    for (int i = 0; i < Membership.rows(); i++)
    {
      Mem[i][k] = Membership[i][k];
    }
  }
  for (int ell = 0; ell < return_item_number(); ell++)
  {
    for (int j = 0; j < ItemMembership.rows(); j++)
    {
      ItemMem[j][ell] = ItemMembership[j][ell];
    }
  }
  // std::cout<<ItemMem<<std::endl;
  // std::cout<<Mem<<std::endl;
  return;
}

void Recom::setMAE_h(void){
  aveMAE_h = DBL_MAX;
  return;
}

void Recom::choiceMAE_h_art(std::vector<std::string> dir, double m,double lam){
  double MAEsum = 0.0;
  for(int i = 0; i < (Current_end - Current_start + 1); i++){
    MAEsum += choiceMAE[0][i];
  }

  //前回の実験の値を追加
  if(Current_start > 0){
    MAEsum += Averages[0] * (double)Current_start;
  }

  double denominator = (double)Current_end + 1.0;

  if(aveMAE_h >= MAEsum / denominator){
    if(aveMAE_h == MAEsum / denominator){
      std::ofstream ofs(dir[0] + "/lowestMae.txt", std::ios::app); //追加
      if (!ofs){
        std::cerr << "ファイルopen失敗: choiceMAE_h\n";
      }
      else{
        ofs << Missing
            << " m=" << m
            << " lam=" << lam
            << " C" << Mem.rows() //クラスタ中心の数
            << std::fixed << std::setprecision(10) << " LowestMAE=" << aveMAE_h
            << std::endl;
      }
    } else {
      aveMAE_h = MAEsum / denominator;
      std::ofstream ofs(dir[0] + "/lowestMae.txt", std::ios::trunc); //上書き
      if (!ofs){
        std::cerr << "ファイルopen失敗: choiceMAE_h\n";
      }
      else{
        ofs << Missing
            << " m=" << m
            << " lam=" << lam
            << " C" << Mem.rows() //クラスタ中心の数
            << std::fixed << std::setprecision(10) << " LowestMAE=" << aveMAE_h
            << std::endl;
      }
    }
  }
  Averages[0] = MAEsum / denominator;
  return;
}

void Recom::choiceMAE_h(void){
  double MAEsum = 0.0;
  for(int i = 0; i < (Current_end - Current_start + 1); i++){
    MAEsum += choiceMAE[0][i];
  }

  //前回の実験の値を追加
  if(Current_start > 0){
    MAEsum += Averages[0] * (double)Current_start;
  }

  double denominator = (double)Current_end + 1.0;

  Averages[0] = MAEsum / denominator;
  return;
}

void Recom::setAUC_h(void){
  aveAUC_h = DBL_MIN;
  return;
}

void Recom::choiceAUC_h_art(std::vector<std::string> dir, double m,double lam){
  double AUCsum = 0.0;
  for(int i = 0; i < (Current_end - Current_start + 1); i++){
    AUCsum += choiceAUC[0][i];
  }

  //前回の実験の値を追加
  if(Current_start > 0){
    AUCsum += Averages[2] * (double)Current_start;
  }

  double denominator = (double)Current_end + 1.0;

  if(aveAUC_h <= AUCsum / denominator){
    if(aveAUC_h == AUCsum / denominator){
      std::ofstream ofs(dir[0] + "/highestAuc.txt", std::ios::app); //追加
      if (!ofs){
        std::cerr << "ファイルopen失敗: choiceAUC_h\n";
      }
      else{
        ofs << Missing
            << " m=" << m
            << " lam=" << lam
            << " C" << Mem.rows() //クラスタ中心の数
            << std::fixed << std::setprecision(10) << " HightestAUC=" << aveAUC_h
            << std::endl;
      }
    } else {
      aveAUC_h = AUCsum / denominator;
      std::ofstream ofs(dir[0] + "/highestAuc.txt", std::ios::trunc); //上書き
      if (!ofs){
        std::cerr << "ファイルopen失敗: choiceAUC_h\n";
      }
      else{
        ofs << Missing
            << " m=" << m
            << " lam=" << lam
            << " C" << Mem.rows() //クラスタ中心の数
            << std::fixed << std::setprecision(10) << " HightestAUC=" << aveAUC_h
            << std::endl;
      }
    }
  }
  Averages[2] = AUCsum / denominator;
  return;
}

void Recom::choiceAUC_h(void){
  double AUCsum = 0.0;
  for(int i = 0; i < (Current_end - Current_start + 1); i++){
    AUCsum += choiceAUC[0][i];
  }

  //前回の実験の値を追加
  if(Current_start > 0){
    AUCsum += Averages[2] * (double)Current_start;
  }

  double denominator = (double)Current_end + 1.0;

  Averages[2] = AUCsum / denominator;
  return;
}

int return_user_number()
{
#ifdef MOVIE
  return 905;
#elif defined MOVIE100K
  return 874;
#elif defined MOVIE10M
  return 1299;
#elif defined NETFLIX
  return 542;
#elif defined BOOK
  return 1091; // 1090;
#elif defined JESTER
  return 2916;
#elif defined MOVIE2
  return 500;
#elif defined LIBIMSETI
  return 866;  // 500;
#elif defined EPINIONS
  return 1022; // 2000;
#elif defined EPINIONS_TWENTY
  return 2088;  
#elif defined EPINIONS_20I_20U
  return 2677;
#elif defined EPINIONS_18I_18U
  return 3548;
#elif defined EPINIONS_15
  return 5531;
#elif defined SUSHI
  return 5000;
#elif defined SUSHI_NINE
  return 3007;
#elif defined SUSHI_450I_7U
  return 2978;
#elif defined ARTIFICIALITY
  return 100;
#elif defined TEST // 動作確認用テストデータ
  return 6;
#elif defined SAMPLE
  return 5;
#else
  std::cout << "Error: In Function \"int return_user_number() (in recom.cxx)\"\n";
  exit(1);
#endif
}

int return_item_number()
{
#ifdef MOVIE
  return 684;
#elif defined MOVIE100K
  return 598;
#elif defined MOVIE10M
  return 1695;
#elif defined NETFLIX
  return 4495;
#elif defined BOOK
  return 2248; // 2247;
#elif defined JESTER
  return 140;
#elif defined MOVIE2
  return 2523;
#elif defined LIBIMSETI
  return 1156; // 2041;
#elif defined EPINIONS
  return 835;  // 517;
#elif defined EPINIONS_TWENTY
  return 1381;  // 517;
#elif defined EPINIONS_20I_20U
  return 2060;
#elif defined EPINIONS_18I_18U
  return 2745;
#elif defined EPINIONS_15
  return 4287;
#elif defined SUSHI
  return 100;
#elif defined SUSHI_NINE
  return 52;
#elif defined SUSHI_450I_7U
  return 31;
#elif defined ARTIFICIALITY
  return 100;
#elif defined TEST // 動作確認用テストデータ
  return 4;
#elif defined SAMPLE
  return 4;
#else
  std::cout << "error recom's func return_item_number\n";
  exit(1);
#endif
}

double return_threshold()
{
#ifdef MOVIE
  return 3.5;
#elif defined MOVIE100K
  return 3.5;
#elif defined MOVIE10M
  return 3.5;
#elif defined NETFLIX
  return 3.5;
#elif defined BOOK
  return 7.0;
#elif defined JESTER
  return 14.7;
#elif defined MOVIE2
  return 3.5;
#elif defined LIBIMSETI
  return 7.0;
#elif defined EPINIONS
  return 3.5;
#elif defined EPINIONS_TWENTY
  return 3.5;
#elif defined EPINIONS_20I_20U
  return 3.5;
#elif defined EPINIONS_18I_18U
  return 3.5;
#elif defined EPINIONS_15
  return 3.5;
#elif defined SUSHI
  return 3.5;
#elif defined SUSHI_NINE
  return 3.5;
#elif defined SUSHI_450I_7U
  return 3.5;
#elif defined ARTIFICIALITY
  return 3.5;
#elif defined TEST // 動作確認用テストデータ
  return 3.0;
#elif defined SAMPLE
  return 3.5;
#else
  std::cout << "error recom's func return_return_threshold\n";
  exit(1);
#endif
}

double return_max_value()
{
#ifdef MOVIE
  return 5.0;
#elif defined MOVIE100K
  return 5.0;
#elif defined MOVIE10M
  return 5.0;
#elif defined NETFLIX
  return 5.0;
#elif defined BOOK
  return 10.0;
#elif defined JESTER
  return 21.0;
#elif defined MOVIE2
  return 5.0;
#elif defined LIBIMSETI
  return 10.0;
#elif defined EPINIONS
  return 5.0;
#elif defined EPINIONS_TWENTY
  return 5.0;
#elif defined EPINIONS_20I_20U
  return 5.0;
#elif defined EPINIONS_18I_18U
  return 5.0;
#elif defined EPINIONS_15
  return 5.0;
#elif defined SUSHI
  return 5.0;
#elif defined SUSHI_NINE
  return 5.0;
#elif defined SUSHI_450I_7U
  return 5.0;
#elif defined ARTIFICIALITY
  return 5.0;
#elif defined TEST // 動作確認用テストデータ
  return 5.0;
#elif defined SAMPLE
  return 5.0;
#else
  std::cout << "error recom's func return_max_value\n";
  exit(1);
#endif
}

double return_median()
{
#ifdef MOVIE
  return 3.0;
#elif defined MOVIE100K
  return 3.0;
#elif defined MOVIE10M
  return 2.5;
#elif defined NETFLIX
  return 3.0;
#elif defined BOOK
  return 5.5;
#elif defined JESTER
  return 0.0;
#elif defined MOVIE2
  return 5.0;
#elif defined LIBIMSETI
  return 5.5;
#elif defined EPINIONS
  return 3.0;
#elif defined EPINIONS_TWENTY
  return 3.0;
#elif defined EPINIONS_20I_20U
  return 3.0;
#elif defined EPINIONS_18I_18U
  return 3.0;
#elif defined EPINIONS_15
  return 3.0;
#elif defined SUSHI
  return 3.0;
#elif defined SUSHI_NINE
  return 3.0;
#elif defined SUSHI_450I_7U
  return 3.0;
#elif defined ARTIFICIALITY
  return 3.0;
#elif defined TEST // 動作確認用テストデータ
  return 3.0;
#elif defined SAMPLE
  return 3.0;
#else
  std::cout << "error recom's func return_max_value\n";
  exit(1);
#endif
}

std::string return_data_name()
{
#ifdef MOVIE
  return "movie";
#elif defined MOVIE100K
  return "movie100k";
#elif defined MOVIE10M
  return "movie10m";
#elif defined NETFLIX
  return "netflix";
#elif defined BOOK
  return "book";
#elif defined JESTER
  return "jester";
#elif defined MOVIE2
  return "movie10M";
#elif defined LIBIMSETI
  return "libimseti";
#elif defined EPINIONS
  return "epinions";
#elif defined EPINIONS_TWENTY
  return "epinions_twenty";
#elif defined EPINIONS_20I_20U
  return "epinions_20i_20u";
#elif defined EPINIONS_18I_18U
  return "epinions_18i_18u";
#elif defined EPINIONS_15
  return "epinions_15ui";
#elif defined SUSHI
  return "sushi";
#elif defined SUSHI_NINE
  return "sushi_nine";
#elif defined SUSHI_450I_7U
  return "sushi_450i_7u";
#elif defined ARTIFICIALITY
  return "artificiality";
#elif defined TEST // 動作確認用テストデータ
  return "test";
#elif defined SAMPLE
  return "sample";
#else
  std::cout << "error recom's func return_data_name\n";
  exit(1);
#endif
}

void FILE_ENDL(std::string text)
{
  std::ofstream ofs(text, std::ios::app);
  ofs << std::endl;
  ofs.close();
  return;
}

void Rename(std::string filename, std::string newname)
{
  if (!access(filename.c_str(), 0))
  { // If the file exists
    // Successfully deleted
    if (!rename(filename.c_str(), newname.c_str()))
    {
      std::cout << "roctxtFile successfully  renamed"
                << std::endl;
      std::cout << newname << std::endl;
    }
    else // Cannot rename: file not open or insufficient permissions
    {
      std::cout
          << "The file cannot be renamed"
          << " (may be the following reasons):"
          << std::endl;
      std::cout << "\t"
                << "1. "
                << newname << " Already exists"
                << std::endl
                << "\t"
                << "2. " << newname
                << " Being used, not closed"
                << std::endl
                << "\t"
                << "3. "
                << "You do not have permission to rename this file"
                << std::endl;
    }
  }
  else
  { // The file does not exist
    std::cout << "Rename failed. This file does not exitst:\n" << filename << std::endl;
  }
  return;
}

std::vector<std::string> MkdirFCCM(std::string method)
{
  std::vector<std::string> v;
  std::string c_p = current_path();
  c_p = c_p + "/../../RESULT";
  mkdir(c_p.c_str(), 0755);
  for (int i = 0; i < (int)FCCM.size(); i++)
  {
    std::string d =
        c_p + "/" + method + "_" + FCCM[i] + "_" + return_data_name() + std::to_string(KESSON);
    mkdir(d.c_str(), 0755);
    v.push_back(d);
  }
  return v;
}

std::vector<std::string> MkdirFCS(std::string method, int kesson)
{
  std::vector<std::string> v;
  std::string c_p = current_path();
  c_p = c_p + "/../../RESULT/" + method;
  mkdir(c_p.c_str(), 0755);
  for (int i = 0; i < (int)FCS.size(); i++)
  {
    std::string d =
        c_p + "/" + method + "_" + FCS[i] + "_" + return_data_name() + std::to_string(kesson);
    mkdir(d.c_str(), 0755);
    v.push_back(d);
  }
  return v;
}

std::vector<std::string> MkdirUBUKATA(std::string method)
{
  std::vector<std::string> v;
  std::string c_p = current_path();
  c_p = c_p + "/../../RESULT";
  mkdir(c_p.c_str(), 0755);
  for (int i = 0; i < (int)UBUKATA.size(); i++)
  {
    std::string d =
        c_p + "/" + method + "_" + UBUKATA[i] + "_" + return_data_name() + std::to_string(KESSON);
    mkdir(d.c_str(), 0755);
    v.push_back(d);
  }
  return v;
}

std::vector<std::string>
Mkdir(std::vector<double> para, int c, std::vector<std::string> dirs)
{
  std::vector<std::string> v;
  std::string fuzzifier = "";
  for (int i = 0; i < (int)para.size(); i++)
  {
    std::ostringstream oss;
    oss << std::setprecision(10) << para[i];
    std::string f(oss.str());
    fuzzifier += f + "_";
  }
  for (int i = 0; i < (int)dirs.size(); i++)
  {
    const std::string dir = dirs[i] + "/" + fuzzifier + "C" + std::to_string(c);
    v.push_back(dir);
    mkdir(dir.c_str(), 0755);
    // predictionフォルダ作成
    const std::string prediction = dir + "/prediction";
    mkdir(prediction.c_str(), 0755);
    // ROCフォルダ作成
    const std::string roc = dir + "/ROC";
    mkdir(roc.c_str(), 0755);
    //選ばれるROCファイルをまとめるフォルダ作成
    const std::string choice = roc + "/choice";
    mkdir(choice.c_str(), 0755);
  }
  return v;
}

std::vector<std::string>
Mkdir(std::vector<std::string> methods, int kesson)
{
  std::vector<std::string> v;
  std::string c_p = current_path();
  c_p = c_p + "/../../RESULT/" + methods[0];
  mkdir(c_p.c_str(), 0755);
  for (int i = 0; i < (int)methods.size(); i++)
  {
    std::string d
    = c_p + "/" + methods[i] + "_" + return_data_name() + std::to_string(kesson);
    mkdir(d.c_str(), 0755);
    // ROCフォルダ作成
    const std::string roc = d + "/ROC";
    mkdir(roc.c_str(), 0755);
    //選ばれるROCファイルをまとめるフォルダ作成
    const std::string choice = roc + "/choice";
    mkdir(choice.c_str(), 0755);
    v.push_back(d);
  }
  return v;
}

std::vector<std::string>
MkdirMF(std::vector<std::string> methods, std::vector<double> para, int kesson)
{
  if(return_user_number() > return_item_number()){
    para[0] = std::round(return_item_number() * para[0] / 100);
  } else {
    para[0] = std::round(return_user_number() * para[0] / 100);
  }
  std::vector<std::string> v;
  std::string c_p = current_path();
  c_p = c_p + "/../../RESULT/" + methods[0];
  mkdir(c_p.c_str(), 0755);
  for (int i = 0; i < (int)methods.size(); i++)
  {
    std::string d
    = c_p + "/" + methods[i] + "_" + return_data_name() + std::to_string(kesson);
    mkdir(d.c_str(), 0755);
    std::string mf_para = "";
    for (int i = 0; i < (int)para.size(); i++)
    {
      std::ostringstream oss;
      oss << std::setprecision(10) << para[i];
      std::string p(oss.str());
      mf_para += p;
      if(i < (int)para.size() - 1)
        mf_para += "_";
    }
    d += "/" + mf_para;
    mkdir(d.c_str(), 0755);
    // ROCフォルダ作成
    const std::string roc = d + "/ROC";
    mkdir(roc.c_str(), 0755);
    //選ばれるROCファイルをまとめるフォルダ作成
    const std::string choice = roc + "/choice";
    mkdir(choice.c_str(), 0755);
    // diffフォルダ作成
    const std::string diff = d + "/diff";
    mkdir(diff.c_str(), 0755);
    v.push_back(d);
  }
  return v;
}

std::string MkdirMF_afterClustering(std::vector<std::string> dir, std::vector<double> para)
{
  std::string mf_para = "";
  for (int i = 0; i < (int)para.size(); i++)
  {
    std::ostringstream oss;
    oss << std::setprecision(10) << para[i];
    std::string p(oss.str());
    mf_para += p;
    if(i == 0){
      mf_para += "p";
    }
    if(i < (int)para.size() - 1)
      mf_para += "_";
  }
  std::string d = dir[0] + "/" + mf_para;
  mkdir(d.c_str(), 0755);
  // ROCフォルダ作成
  const std::string roc = d + "/ROC";
  mkdir(roc.c_str(), 0755);
  // diffフォルダ作成
  const std::string diff = d + "/diff";
  mkdir(diff.c_str(), 0755);
  return d;
}

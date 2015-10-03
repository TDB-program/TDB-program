// TestTDBAPI_v2.cpp : Defines the entry point for the console application.
//

#include "Platform.h"
#include "PathHelper.h"

#ifdef __PLATFORM_WINDOWS__
#include "stdafx.h"
#include <windows.h>
#else
#include <stdarg.h>
#endif

#include "TDBAPI.h"
#include <sstream>
#include <vector>
#include <utility>
#include <assert.h>
#include <string>
#include <time.h>
#include <fstream>
#include <direct.h>
using namespace std;
#define ELEMENT_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))


int Print(const char* szFormat, ...);   //������Խ���������������


#define TICK_BEGIN(Topic) \
	int nBefore##Topic##Tick_ = GetTickCount();

#define TICK_END(Topic) \
	int nAfter##Topic##Tick_ = GetTickCount(); \
	Print("Topic:%s time-sep:%d\n", #Topic, nAfter##Topic##Tick_ - nBefore##Topic##Tick_);

void TestLogIn(const char* szIP, const char* szPort, const char*szUser, const char* szPassword );
void TestRetrieveData(THANDLE hTdb);


std::string arr2str(int arr[], int n);



void GetTick(THANDLE hTdb, const std::string& strCode, bool bWithAB, int nStartDay, int nEndDay, int nStartTime =0, int nEndTime = 0);
void GetK(THANDLE hTdb, const std::string& strCode, CYCTYPE nCycType,int nCycDef, REFILLFLAG nFlag,  int nAutoComplete, int nStartDay, int nEndDay, int nStartTime=0, int nEndTime = 0);


void GetTransaction(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime=0, int nEndTime=0);
void GetOrder(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime=0, int nEndTime=0);
void GetOrderQueue(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime=0, int nEndTime=0);
void TestFormula(THANDLE hTdb);

void DynamicData(THANDLE hTdb,const string factor, vector<int> testStartDate,vector<string> stockList,vector<vector<string>>& rankStockList);
void Oscillator(THANDLE hTdb, const std::string& strCode,int momCyc, double& oscillator);
void Volatility(THANDLE hTdb, const std::string& strCode, double& volatility);
bool getOneMomentum(THANDLE hTdb,const string strCode,int startDate,int endDate,double& mom);//���㶯��

void LoadIPFromCin(OPEN_SETTINGS&);

int getLastTradeDate(THANDLE hTdb,const std::string& strCode);
int getStarDatetFromInterval(int startDate,int interval);

bool getOneProfit(THANDLE hTdb,int startTime,int endTime,const string& stockid,double& oneProfit,double& baseProfit);
void getData(string factor,vector<vector<string>>& stockList,char factorType);
void getTestDate(vector<int>& testStartDate);
void computeProfit(THANDLE hTdb,vector<int> time,vector<vector<string>>stockList,vector <vector<bool>>& MarketVSGroup,int numQuater,int groupNumber,vector<double>& profit);

int main(int argc, char* argv[])
{   
	OPEN_SETTINGS settings={"127.0.0.1","10222","user","pwd",30,2,0}; // ��¼ʱ����Ϣ,�������ֵ
	LoadIPFromCin(settings); // �����¼��Ϣ
	TDBDefine_ResLogin loginAnswer={0}; 
	
	THANDLE hTdb = TDB_Open(&settings, &loginAnswer);
	if (!hTdb)
	{ 
		Print("TDB_Open failed:%s, program exit!\n", loginAnswer.szInfo);
		exit(0);
	}
	/*
	double shProfit=0.0;
	getOneProfit(hTdb,20140601,20140901,"399001.sz",shProfit);
	cout<<shProfit<<endl;
	system("pause");
	return 0;
	*/
	//string strCode="000820.sz";
	//string strCode="601857.sh";
	//GetK(hTdb, strCode, CYC_DAY, 1, REFILL_NONE, 0, 20140601, 20140901);
	
	double oscillator=0.0;
	double volatility=0.0;
	//string strCode="601857.sh";
	//Momentum(hTdb,strCode,12,monentum);
	//Oscillator(hTdb, strCode,1, oscillator);
	//Volatility(hTdb, strCode, volatility);
	int groupNumber=7;
	int numQuater=7;
	string factorName="MOM6";
	char factorType='D';
	vector<double> profit(groupNumber,0.0);
	vector<vector<string>>TmpstockList;
	vector<vector<string>>rankStockList;
	vector<int> testStartDate; 
	getTestDate(testStartDate); //��ʼ��ʱ������
	vector <vector<bool>> MarketVSGroup(numQuater,vector<bool>(groupNumber,false));   //7�����ȣ�ÿ�����������Ƿ���Ӯ����
	//-------------���㶯̬����-------------------------------------
	//����ֻ��Ҫ����һ��
	getData("PE",TmpstockList,'S');
	DynamicData(hTdb,factorName,testStartDate,TmpstockList[0],rankStockList); //���ݶ������ӻ�ȡ��Ʊ�б�
	//����ֻ��Ҫ����һ��
	vector<vector<string>>stockList;
	getData(factorName,stockList,factorType);
	
	computeProfit(hTdb,testStartDate,stockList,MarketVSGroup,numQuater,groupNumber,profit);

	for(int i=0;i<groupNumber;i++){
		cout<<"��"<<i<<"���ӯ����Ϊ"<<profit[i];
		int number=0;
		for(int j=0;j<numQuater;j++){
		     if(MarketVSGroup[j][i]==1) number++;
		}
		cout<<" ��Ӯ�г��ĸ���Ϊ��"<<number*1.0/numQuater<<endl;
	}
	
	system("pause");
	
	return 0;

}

//input:ÿ�����ȵĸ��������ź����stockList 7*2000 ������
//time:8�����ȿ�ʼ��ʱ�䣬������0�����ȵĿ�ʼʱ��
//groupNumber���������Ŀ
//output:ÿһ������������
//over
double szProfitRate=0.0;
int pec=0;
void computeProfit(THANDLE hTdb,vector<int> time,vector<vector<string>>stockList,vector <vector<bool>>& MarketVSGroup,int numQuater,int groupNumber,vector<double>& profit){
    vector<vector<double>>nums(numQuater,vector<double>(groupNumber,0.0)); //nums[����][�����]=ĳ����ĳ�����ӯ����
	vector<vector<double>>numsRate(numQuater,vector<double>(groupNumber,0.0)); //numsRate[����][�����]=ĳ����ĳ�����������
	vector<double> marketTndexProfit(numQuater,0.0); //���̵�ÿ������������
	
	for(int i=0;i<numQuater;i++){
		cout<<"��"<<i<<"�������ڼ���"<<endl;
		cout<<stockList[i].size()<<endl;
		
		int startTime=time[i+1];
		int endTime=time[i];
		double szProfit=0.0;
		double szBaseProfit=0.0;
		//double szProfitRate=0.0;
		//getOneProfit(hTdb,startTime,endTime,"999999.sh",shProfit);//������֤���д��̸ü��ȵ�ӯ��״��
		getOneProfit(hTdb,startTime,endTime,"399001.sz",szProfit,szBaseProfit);//������֤���д��̸ü��ȵ�ӯ��״��
		szProfitRate=szProfit/szBaseProfit;
		//cout<<"��֤��ָ"<<shProfit<<endl;
		cout<<"��֤��ָ"<<szProfitRate<<endl;
		/*
		double baseProfit=0.0,oneProfit=0.0;
		for(int k=0;k<stockList[i].size();k++)
		     getOneProfit(hTdb,startTime,endTime,stockList[i][k],oneProfit,baseProfit);
		*/
		
		marketTndexProfit[i]=szProfitRate;
        int start=0;
		int k=0; //ÿ������Ŀ�ʼλ��
		for(int j=0;j<groupNumber;j++){
			   
			   int number=0;        
			   double baseProfit=0.0;  //ÿ��Ŀ�ʼ���ʽ�
			   for(k=start;number<20 && k<stockList[i].size();k++){  //��ȡ10ֻ��ƱΪ1��
						 double oneProfit=0.0;  //��ֻ��Ʊ��ӯ��
						 
						 if(getOneProfit(hTdb,startTime,endTime,stockList[i][k],oneProfit,baseProfit)){
							 nums[i][j]+=oneProfit;
							 number++;
						 }
			   }
			   cout<<i<<"����"<<j<<"����Ĺ�Ʊ��:"<<number<<" ";
			   start=k+100;     //ÿһ��������100��ȷ���������������ֵ����Ƚϴ�
			   cout<<nums[i][j]<<" / "<<baseProfit<<endl;     //�����ӯ��/Ͷ�ʶ�
			   numsRate[i][j] = nums[i][j]/baseProfit; //�÷�����껯������
			   cout<<numsRate[i][j]<<endl;
		}
		
		cout<<double(pec)/(20*groupNumber)<<endl;//���㵥ֻ��Ʊ��Ӯ�г��ĸ���
		szProfitRate=0.0;
		pec=0;
		
	}
	cout<<"��ʼ������������ӯ������"<<endl;

	for(int i=0;i<numQuater;i++){
		for(int j=0;j<groupNumber;j++){
		     profit[j]+=nums[i][j];
			 if(numsRate[i][j]>marketTndexProfit[i])
				 MarketVSGroup[i][j]=true;
			 cout<<"���ȣ�"<<i<<"����:"<<j<<"��Ӯ���г���"<<MarketVSGroup[i][j]<<endl;
		}
	}
}

//jop:����֧��Ʊ�� һ��ʱ����ڵ�ӯ�����ټ������
//���룺ʱ��ο�ʼʱ�� ʱ��ν���ʱ�� ��Ʊ����
//�����ӯ���� ���TDB�в����ڸù�Ʊ��Ϣ�򷵻�false
//over
bool getOneProfit(THANDLE hTdb,int startTime,int endTime,const string& stockid,double& oneProfit,double& baseProfit){
	TDBDefine_ReqKLine reqK = {"", REFILL_NONE, 0, 0, CYC_DAY, 1, 1, startTime, endTime, 0, 0};
	strncpy(reqK.chCode, stockid.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount =0;
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	if(nCount==0){
		//cout<<"δ��ѯ���ù�Ʊ"<<stockid<<endl;
		return false;
	}
	//cout<<"��ѯ���ù�Ʊ"<<stockid<<endl;
	TDBDefine_KLine& buyTDB = *(pKLine); 
	TDBDefine_KLine& saleTDB = *(pKLine + nCount-1);
	oneProfit=(saleTDB.nClose-buyTDB.nOpen)*1.0/10000;
	double rate=oneProfit*10000/buyTDB.nOpen;
	if(rate > szProfitRate)
		pec++;
	baseProfit+=buyTDB.nOpen/10000;
	TDB_Free(pKLine);
	return true;
}

//jop: ��csv�ļ����ݵ��룬����Ʊ�����ʽת��ΪTDB��ʽ
//���룺�������� factor,�������� S��static D:dynamic
//�������Ʊ�б� 7*2000������ ��������ֵ�ź����stockid
//over
void getData(string factor,vector<vector<string>>& stockList,char factorType){
    ifstream ifs;
	istringstream line_stream;
	for(int i=1;i<=7;i++){
	   vector<string> subStockList;
	   ifs.open(factor+"/"+to_string(long long (i))+".csv");
	   string stockid;
	   string line;
	   int j=0;
	   while(getline(ifs,line,'\n')){
           //cout << line << endl;
           line_stream.str(line); // set the input stream to line
           getline(line_stream, stockid, ','); // reads from line_stream and writes to stockid
		   //��stockidת��ΪTDB�ĸ�ʽ
		    string TDBstockid=stockid;

           if(factorType=='S'){  //���Ϊ��̬��������Ҫ����Ʊ����ת��ΪTDB�Ĺ�Ʊ��������  �´���python���޸Ĺ�Ʊ����
		       string type=stockid.substr(0,2);
		       if(type=="sh")  //ֻȡ���ڹ��й�Ʊ����
			        continue;
		       TDBstockid=stockid.substr(2,6)+'.'+type;
		   }

		   subStockList.push_back(TDBstockid);
	   }
	   stockList.push_back(subStockList);
	   ifs.close();    //�ǵùر��ļ��������޷�������һ���ļ�
	}
	for(int i=0;i<stockList.size();i++)
		cout<<"��"<<i+1<<"����"<<stockList[i].size()<<endl;
}

//jop:��ȡ���Լ�ÿ�����ȵĿ�ʼʱ�䣬ע�����ڱ���Ϊ������
//���룺
//�����һ��һάvector �洢�� i �����ȵĿ�ʼ����
void getTestDate(vector<int>& testStartDate){
	testStartDate.push_back(20140901);    //0����
	testStartDate.push_back(20140601);    //1����
	testStartDate.push_back(20140301);
	testStartDate.push_back(20140101);
	testStartDate.push_back(20130901);
	testStartDate.push_back(20130601);
	testStartDate.push_back(20130301);
	testStartDate.push_back(20130101);

}
//���һ�����������յ�����
int getLastTradeDate(THANDLE hTdb,const std::string& strCode){
	time_t rawtime;
	struct tm * ptm;

	time(&rawtime);
	ptm = gmtime(&rawtime);

	//�����һ��25�ŵ��������������
	int enddate=(ptm->tm_year+1900)*10000 + (ptm->tm_mon+1)*100 + ptm->tm_mday;
	int startdate=(ptm->tm_year+1900)*10000 + (ptm->tm_mon==0?12:ptm->tm_mon)*100 +25;

	TDBDefine_ReqKLine reqK = {"", REFILL_NONE, 0, 0, CYC_DAY, 1, 1, startdate, enddate, 0, 0};
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));

	TDBDefine_KLine* pKLine = NULL;
	int nCount =0;
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	TDBDefine_KLine& tdbK = *(pKLine + nCount-1); //��������Ľ��������ڣ��������һ���������Ƿ�����
	int ResDate=tdbK.nDate;
	TDB_Free(pKLine);
	return ResDate;
}

//���ݽ���ʱ�䣬ʱ���������㿪ʼʱ��
//���룺����ʱ�䣬ʱ����(��)
//�������ʼʱ��
int getStarDatetFromInterval(int endDate,int interval){
	int month=endDate%10000/100;
	int day=endDate%100;
	if(month>interval)
		return endDate-interval*100;
	else{
	    interval-=month;
		int year=endDate/10000;
		year-=interval/12+1;
		return year*10000+(12-interval%12)*100+day;
	}
}

void GetTick(THANDLE hTdb, const std::string& strCode, bool bWithAB, int nStartDay, int nEndDay, int nStartTime/* =0*/, int nEndTime/* = 0*/)
{
	TDBDefine_ReqTick reqTick = {"",nStartDay, nEndDay, nStartTime, nEndTime};
	strncpy(reqTick.chCode, strCode.c_str(), sizeof(reqTick.chCode));

	if (!bWithAB)
	{
		TDBDefine_Tick* pTick = NULL;
		int nCount;
		int nRet = TDB_GetTick(hTdb, &reqTick, &pTick, &nCount);

		Print("---------------------�յ�%d����գ�������:%d -----------------\n", nCount,nRet);
		for (int i=0; i<nCount && i<10; i++)
		{
			TDBDefine_Tick& tdbTick = *(pTick+i);
#ifdef __PLATFORM_WINDOWS__
			Print("code:%s, date:%d, time:%d, nprice:%d, vol:%I64d, turover:%I64d, acc vol:%I64d, acc turover:%I64d\n", tdbTick.chWindCode, tdbTick.nDate, tdbTick.nTime, tdbTick.nPrice, tdbTick.iVolume, tdbTick.iTurover, tdbTick.iAccVolume, tdbTick.iAccTurover);
#else
			Print("code:%s, date:%d, time:%d, nprice:%d, vol:%lld, turover:%lld, acc vol:%lld, acc turover:%lld\n", tdbTick.chWindCode, tdbTick.nDate, tdbTick.nTime, tdbTick.nPrice, tdbTick.iVolume, tdbTick.iTurover, tdbTick.iAccVolume, tdbTick.iAccTurover);
#endif

		}
		TDB_Free(pTick);
	}
	else
	{

		TDBDefine_TickAB* pTick = NULL;
		int nCount;
		int nRet = TDB_GetTickAB(hTdb, &reqTick, &pTick, &nCount);

		Print("---------------------�յ�%d����գ�������:%d -----------------\n", nCount, nRet);
		for (int i=0; i<nCount  && i<10; i++)
		{
			TDBDefine_TickAB& tdbTick = *(pTick+i);
			//if(20130409 != tdbTick.nDate)
#ifdef __PLATFORM_WINDOWS__
			Print("code:%s, date:%d, time:%d, nprice:%d, vol:%I64d, turover:%I64d, acc vol:%I64d, acc turover:%I64d\n", tdbTick.chWindCode, tdbTick.nDate, tdbTick.nTime, tdbTick.nPrice, tdbTick.iVolume, tdbTick.iTurover, tdbTick.iAccVolume, tdbTick.iAccTurover);
#else
			Print("code:%s, date:%d, time:%d, nprice:%d, vol:%lld, turover:%lld, acc vol:%lld, acc turover:%lld\n", tdbTick.chWindCode, tdbTick.nDate, tdbTick.nTime, tdbTick.nPrice, tdbTick.iVolume, tdbTick.iTurover, tdbTick.iAccVolume, tdbTick.iAccTurover);
#endif

		}
		TDB_Free(pTick);
	}

}

typedef pair<string,double> PAIR;
struct CmpByValue {  
  bool operator()(const PAIR& lhs, const PAIR& rhs) {  
    return lhs.second > rhs.second;  
  }  
};  

bool getOneMomentum(THANDLE hTdb,const string strCode,int startDate,int endDate,double& mom){
	TDBDefine_ReqKLine reqK = { "", REFILL_NONE, 0, 0, CYC_DAY,1,1, startDate, endDate,0,0 };
	
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount = 0;

	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	if(nCount ==0)
		return false;
    TDBDefine_KLine& buyTDB = *(pKLine); 
	TDBDefine_KLine& saleTDB = *(pKLine + nCount-1);
	
	mom = (saleTDB.nClose-buyTDB.nOpen)*1.0/10000;
	TDB_Free(pKLine);
	return true;
}
//���ݶ�̬���ӻ�ȡ�������ȵĹ�Ʊ�����б�
//input�����ݿ⣺htdb  ʱ��Σ�testStartDate(7������) ��Ʊ�б�7*2000������ֵ):stockList
//output: void
void DynamicData(THANDLE hTdb,const string factor,vector<int> testStartDate,vector<string> stockList,vector<vector<string>>& rankStockList){
	_mkdir(factor.c_str());   //�����ļ��д洢����
	ofstream ofs;
	for(int i=0;i<7;i++){
		 ofs.open(factor+"/"+ to_string(long long (i+1))+".csv");
         vector<PAIR> stock_mom_pair;
		 int startDate=getStarDatetFromInterval(testStartDate[i],6);
		 for(int j=0;j<stockList.size();j++){
			  double mom;
			  //��ȡ�ù�Ʊn���µĶ���  
		      if(getOneMomentum(hTdb,stockList[j],startDate,testStartDate[i],mom)==false)
				  continue;
			  PAIR a(stockList[j],mom);
			  stock_mom_pair.push_back(a);
		 }
		 cout<<"��ʼ����"<<endl;
		 sort(stock_mom_pair.begin(), stock_mom_pair.end(), CmpByValue());
		 cout<<"�������"<<endl;
		 for(auto iter = stock_mom_pair.begin(); iter!=stock_mom_pair.end(); iter++)
         {  
              cout<<iter->first<<" : "<<iter->second<<endl;
			  //substock.push_back(iter->first);
			  ofs<<iter->first<<","<<iter->second<<endl;
         }
		 ofs.close();
		 //rankStockList.push_back(substock);
	}
}

//������ָ��
//��߼�-��ͼ�/���̼�+���̼� =�ö�ʱ�����ָ��
//input: ���ݿ⣺htdb   ��Ʊ���ţ�strCode  ʱ���ȣ�momCyc (1~12 �£��ӽ��쿪ʼ����ʱ��)  ��ָ�꣨����ֵ����oscillator
//output: void
void Oscillator(THANDLE hTdb, const std::string& strCode,int momCyc, double& oscillator){
	int nStartDay, nEndDay;
	nEndDay=getLastTradeDate(hTdb,strCode);
	nStartDay=nEndDay-10000;//һ���ʱ��
	TDBDefine_ReqKLine reqK = { "", REFILL_NONE, 0, 0, CYC_MONTH,momCyc,1, nStartDay, nEndDay,0,0 };
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount;
	TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	TDBDefine_KLine* res=pKLine+nCount-1;
	cout<<res->nDate;
	oscillator = double(res->nHigh - res->nLow) / (res->nOpen + res->nClose);
	system("pause");
	TDB_Free(pKLine);
}

//�����Ʊһ��Ĳ�����
//���� log(ÿ�����̼�/ÿ�տ��̼�) ��ƽ���� * һ�꽻����������ƽ����
//input:���ݿ�:hTdb  ��Ʊ����:strCode  һ��Ĳ�����(��Ϊ����ֵ):volatility
//output:void
void Volatility(THANDLE hTdb, const std::string& strCode, double& volatility)
{
	int nStartDay, nEndDay;
	nEndDay=getLastTradeDate(hTdb,strCode);
	nStartDay=nEndDay-10000;//һ���ʱ��
	TDBDefine_ReqKLine reqK = { "", REFILL_NONE, 0, 0, CYC_DAY,1,1, nStartDay, nEndDay,0,0 };
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount = 0;
	//TICK_BEGIN(minitue);1
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	vector<double> tmp(nCount,0.0);
	//TICK_END(minitue);
	Print("---------------------�յ�%d��K�ߣ�������:%d -----------------\n", nCount, nRet);
	double sum = 0.0;
	for (int i = 0; i < nCount; i++) 
	{
		TDBDefine_KLine& tdbK = *(pKLine + i);
		tmp[i] = log((double)tdbK.nClose / tdbK.nOpen);
		sum += tmp[i];
	}
	double average = (double)sum / nCount;
	sum = 0.0;
	for (int i = 0; i < nCount; i++)
	{
		sum += pow(tmp[i]-average,2);
	}
	volatility = sqrt(sum/nCount) * sqrt(double(nCount));
	cout<<volatility;
	system("pause");
	TDB_Free(pKLine);
}

void GetK(THANDLE hTdb, const std::string& strCode, CYCTYPE nCycType, int nCycDef, REFILLFLAG nFlag,  int nAutoComplete, int nStartDay, int nEndDay, int nStartTime/*=0*/, int nEndTime/* = 0*/)
{
	TDBDefine_ReqKLine reqK = {"", nFlag, 0, 0, nCycType, nCycDef, nAutoComplete, nStartDay, nEndDay, nStartTime, nEndTime};
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount =0;
	//TICK_BEGIN(minitue);1
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	//TICK_END(minitue);
	Print("---------------------�յ�%d��K�ߣ�������:%d -----------------\n", nCount, nRet);
	//ofstream outfile;
	//outfile.open("KLinefile.csv", ios::app);
	//outfile.open("GetK.csv");
	//outfile << "��Ʊ����" << "," << "����" << "," << "ʱ��" << "," << "���̼�" << "," << "��߼�" << "," <<"��ͼ�" << "," << "���̼�" << "," <<"�ɽ���" << "," << "iTurover" << endl;
	for (int i=0; i<nCount; i++) // ֻ�����ǰ10��
	{
		if (i % 1000000 == 0)
			cout << i << endl;
		TDBDefine_KLine& tdbK = *(pKLine+i);
#ifdef __PLATFORM_WINDOWS__
		//std::cout << "code:" << tdbK.chWindCode <<", date:" <<  tdbK.nDate <<", nTime:" << tdbK.nTime << std::endl;
		//outfile << tdbK.chWindCode << "," << tdbK.nDate << "," << tdbK.nTime << "," << tdbK.nOpen << "," << tdbK.nHigh << "," << tdbK.nLow << "," << tdbK.nClose << "," << tdbK.iVolume << "," << tdbK.iTurover<< endl;
		Print("code:%s, date:%d, time:%d, open:%d, high:%d, low:%d, close:%d, volume:%I64d, turover:%I64d\n", tdbK.chWindCode, tdbK.nDate, tdbK.nTime, tdbK.nOpen, tdbK.nHigh, tdbK.nLow, tdbK.nClose, tdbK.iVolume, tdbK.iTurover);
#else
		Print("code:%s, date:%d, time:%d, open:%d, high:%d, low:%d, close:%d, volume:%lld, turover:%lld\n", tdbK.chWindCode, tdbK.nDate, tdbK.nTime, tdbK.nOpen, tdbK.nHigh, tdbK.nLow, tdbK.nClose, tdbK.iVolume, tdbK.iTurover);
#endif

	}
	//outfile.close();
	system("pause");
	TDB_Free(pKLine);
}


int Print(const char* szFormat, ...)
{
	const int MAX_OUTPUT_LEN = 65534;
	int nBufSize = MAX_OUTPUT_LEN+1;
	va_list vArgs;
	va_start(vArgs, szFormat);

	char* szBuf = new char [nBufSize];
	vsnprintf_s(szBuf, nBufSize,nBufSize-1 , szFormat, vArgs);
	va_end(vArgs);

	DebugPrint(szBuf);

	printf(szBuf);

	delete[] szBuf;
	return 0;
}

void GetTransaction(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime/*=0*/, int nEndTime/*=0*/)
{
	TDBDefine_ReqTransaction req = {"", nStartDay, nEndDay, nStartTime, nEndTime};
	strncpy(req.chCode, strCode.c_str(), sizeof(req.chCode));

	TDBDefine_Transaction* pData = NULL;
	int nCount = 0;
	int nRet = TDB_GetTransaction(hTdb, &req, &pData, &nCount);
	Print("--------�յ�%d����ʳɽ���nRet:%d -------\n", nCount, nRet);
	for (int i=0; i<nCount && i<100; i++)
	{
		TDBDefine_Transaction& transaction = pData[i];
		Print("code:%s, date:%d, time:%d, function_code:%c, chBSFlag:%c, price:%d, volume:%d\n",transaction.chWindCode, transaction.nDate, transaction.nTime, transaction.chFunctionCode?transaction.chFunctionCode:' ', transaction.chBSFlag?transaction.chBSFlag:' ', transaction.nTradePrice, transaction.nTradeVolume);
	}
	system("pause");
	TDB_Free(pData);
}

void GetOrder(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime/*=0*/, int nEndTime/*=0*/)
{
	TDBDefine_ReqOrder req = {"", nStartDay, nEndDay, nStartTime, nEndTime};
	strncpy(req.chCode, strCode.c_str(), sizeof(req.chCode));

	TDBDefine_Order* pData = NULL;
	int nCount = 0;
	int nRet = TDB_GetOrder(hTdb, &req, &pData, &nCount);
	Print("--------�յ�%d�����ί�У�nRet:%d -------\n", nCount, nRet);
	for (int i=0; i<nCount && i<10; i++)
	{
		TDBDefine_Order& order = pData[i];
		Print("code:%s, date:%d, time:%d, chOrderKind:%c, chFunctionCode:%c, price:%d, volume:%d\n",order.chWindCode, order.nDate, order.nTime, order.chOrderKind?order.chOrderKind:' ', order.chFunctionCode?order.chFunctionCode:' ', order.nOrderPrice, order.nOrderVolume);
	}
	TDB_Free(pData);
}
void GetOrderQueue(THANDLE hTdb, const std::string& strCode, int nStartDay, int nEndDay, int nStartTime/*=0*/, int nEndTime/*=0*/)
{
	TDBDefine_ReqOrderQueue req = {"", nStartDay, nEndDay, nStartTime, nEndTime};
	strncpy(req.chCode, strCode.c_str(), sizeof(req.chCode));

	TDBDefine_OrderQueue* pData = NULL;
	int nCount = 0;
	int nRet = TDB_GetOrderQueue(hTdb, &req, &pData, &nCount);
	Print("--------�յ�%d��ί�ж��У�nRet:%d -------\n", nCount, nRet);
	for (int i=0; i<nCount && i<10; i++)
	{
		TDBDefine_OrderQueue& orderQueue = pData[i];
		Print("code:%s, date:%d, time:%d, nSide:%c, price:%d, order items:%d, ab items:%d, ab volume:%s\n", orderQueue.chWindCode, orderQueue.nDate, orderQueue.nTime, orderQueue.nSide?orderQueue.nSide:' ', orderQueue.nPrice, orderQueue.nOrderItems, orderQueue.nABItems, arr2str(orderQueue.nABVolume, orderQueue.nABItems).c_str());
	}
	TDB_Free(pData);
}

std::string arr2str(int arr[], int n)
{
	std::string str;
	str.reserve(32*n);
	for (int i=0; i<n; i++)
	{
		char szBuf[32] = {0};
		_snprintf(szBuf, sizeof(szBuf)-1, "%d", arr[i]);
		str = str + szBuf + " ";
	}
	return str;
}


//�ϴ����ؼ�����й�ʽ
void TestFormula(THANDLE hTdb)
{
	const char* pKDJFormula = 
		"INPUT:N(9), M1(3,1,100,2), M2(3);    "
		"RSV:=(CLOSE-LLV(LOW,N))/(HHV(HIGH,N)-LLV(LOW,N))*100;    "
		"K:SMA(RSV,M1,1);    "
		"D:SMA(K,M2,1);    "
		"J:3*K-2*D;    ";

	const char* pCloseFormula = "CLOSE;    ";

	const char* pSlideAveLine = 
		"INPUT:M1(5,2,999), M2(10,2,999), M3(30,2,999), M4(60,2,999);    "
		"MA1:MA(CLOSE,M1);    "
		"MA2:MA(CLOSE,M2);    "
		"MA3:MA(CLOSE,M3);    "
		"MA4:MA(CLOSE,M4);    ";

	const char* pMACDFormula = 
		"INPUT:SHORT(12,2,200,2), LONG(26,2,200,2), MID(9,2,200,2);    "
		"DIF:EMA(CLOSE,SHORT)-EMA(CLOSE,LONG);    "
		"DEA:EMA(DIF,MID);    "
		"MACD:(DIF-DEA)*2,COLORSTICK;    ";

	struct 
	{
		const char* szFormulaName;
		const char* szFormulaText;
	} arrFomula[] = 
	{
		"KDJ",              pKDJFormula,
		"Close",            pCloseFormula,
		"SlideAveLine",     pSlideAveLine,
		"MACD",             pMACDFormula,
	};

	for (int i=0; i<sizeof(arrFomula)/sizeof(arrFomula[0]); i++)
	{
		//���ع�ʽ
		TDBDefine_AddFormulaRes resFormula = {0};
		int nRet = TDB_AddFormula(hTdb, arrFomula[i].szFormulaName, arrFomula[i].szFormulaText, &resFormula);
		printf("�ϴ���ʽ��nRet:%d, err line:%d, text:%s, info:%s\n", nRet, resFormula.nErrLine, resFormula.chText, resFormula.chInfo);
	}

	//��ȡ�Ѿ����صĹ�ʽ
	TDBDefine_FormulaItem* pFormula = NULL;
	int nFormulaCount = 0;
	int nRet = TDB_GetFormula(hTdb, "", &pFormula, &nFormulaCount);
	for (int i=0; i<nFormulaCount; i++)
	{
		printf("��ȡ��ʽ��nRet:%d, count:%d, name:%s, param:%s\n", nRet, nFormulaCount,pFormula[i].chFormulaName, pFormula[i].chParam);
	}

	//���������
	TDBDefine_CalcFormulaRes calcResult = {0};
	TDBDefine_ReqCalcFormula calcReq = {"","", "000001.sz", CYC_MINUTE, 1, REFILL_NONE, 0, 0, 4000, 400};
	//pFormula[0].chFormulaName,  pFormula[0].chParam
	const int nFormulaIndex = 3;

	//��calcReq ��ȱ��ǰ�����ַ������� �ֱ���Ҫ���㹫ʽ������Ҫ�Ĳ���
	strncpy(calcReq.chFormulaName, pFormula[3].chFormulaName, sizeof(calcReq.chFormulaName));
	strncpy(calcReq.chParam, pFormula[3].chParam, sizeof(calcReq.chParam));

	nRet = TDB_CalcFormula(hTdb, &calcReq, &calcResult);

	printf("�������ָ�깫ʽ�����nRet:%d, record count:%d, filed count:%d\n", nRet, calcResult.nRecordCount, calcResult.nFieldCount);

	ofstream outfile;
	outfile.open("Calcfile.csv", ios::app);
	for (int i=0; i<calcResult.nFieldCount; i++)
	{
		outfile << calcResult.chFieldName[i] << ",";
		printf("%s ", calcResult.chFieldName[i]);
	}
	outfile << endl;
	printf("\n");

	for (int i=0; i<calcResult.nRecordCount; i++)
	{
		printf("--------%d----------\n", i+1);
		for (int j = 0; j<calcResult.nFieldCount; j++)
		{
			outfile << calcResult.dataFileds[j][i] << ",";
			printf("%d ", calcResult.dataFileds[j][i]);
		}
		outfile << endl;
		printf("\n");
	}
	outfile.close();
	TDB_ReleaseCalcFormula(&calcResult);
	TDB_Free(pFormula);

	//ɾ��ָ�깫ʽ
	TDBDefine_DelFormulaRes pDelRes = {0};
	nRet = TDB_DeleteFormula(hTdb, "abc", &pDelRes);//
	printf("ɾ����ʽ%s: nRet:%d, ��Ϣ��%s\n",  pDelRes.chFormularName,nRet, pDelRes.chInfo);
}

void LoadIPFromCin(OPEN_SETTINGS& settings)
{
	strcpy(settings.szIP, "114.80.154.34");
    strcpy(settings.szPort, "10061");
    strcpy(settings.szUser, "TD1033863003");
    strcpy(settings.szPassword, "46005604");
	/*std::string strLine;
	printf("Input IP:");
	std::getline(std::cin, strLine);
	strncpy(settings.szIP, strLine.c_str(), sizeof(settings.szIP)-1);
	settings.szIP[ELEMENT_COUNT(settings.szIP)-1] = 0;

	printf("Input Port:");
	std::getline(std::cin, strLine);
	strncpy(settings.szPort, strLine.c_str(), sizeof(settings.szPort)-1);
	settings.szPort[ELEMENT_COUNT(settings.szPort)-1] = 0;

	printf("Input User:");
	std::getline(std::cin, strLine);
	strncpy(settings.szUser, strLine.c_str(), sizeof(settings.szUser)-1);
	settings.szUser[ELEMENT_COUNT(settings.szUser)-1] = 0;

	printf("Input Password:");
	std::getline(std::cin, strLine);
	strncpy(settings.szPassword, strLine.c_str(), sizeof(settings.szPassword)-1);
	settings.szPassword[ELEMENT_COUNT(settings.szPassword)-1] = 0;*/
}

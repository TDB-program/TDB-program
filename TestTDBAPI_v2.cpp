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


int Print(const char* szFormat, ...);   //代表可以接任意多个参数。。


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
bool getOneMomentum(THANDLE hTdb,const string strCode,int startDate,int endDate,double& mom);//计算动量

void LoadIPFromCin(OPEN_SETTINGS&);

int getLastTradeDate(THANDLE hTdb,const std::string& strCode);
int getStarDatetFromInterval(int startDate,int interval);

bool getOneProfit(THANDLE hTdb,int startTime,int endTime,const string& stockid,double& oneProfit,double& baseProfit);
void getData(string factor,vector<vector<string>>& stockList,char factorType);
void getTestDate(vector<int>& testStartDate);
void computeProfit(THANDLE hTdb,vector<int> time,vector<vector<string>>stockList,vector <vector<bool>>& MarketVSGroup,int numQuater,int groupNumber,vector<double>& profit);

int main(int argc, char* argv[])
{   
	OPEN_SETTINGS settings={"127.0.0.1","10222","user","pwd",30,2,0}; // 登录时的信息,并赋予初值
	LoadIPFromCin(settings); // 输入登录信息
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
	getTestDate(testStartDate); //初始化时间数组
	vector <vector<bool>> MarketVSGroup(numQuater,vector<bool>(groupNumber,false));   //7个季度，每个分组增速是否跑赢大盘
	//-------------计算动态因子-------------------------------------
	//以下只需要计算一次
	getData("PE",TmpstockList,'S');
	DynamicData(hTdb,factorName,testStartDate,TmpstockList[0],rankStockList); //根据动量因子获取股票列表
	//以上只需要计算一次
	vector<vector<string>>stockList;
	getData(factorName,stockList,factorType);
	
	computeProfit(hTdb,testStartDate,stockList,MarketVSGroup,numQuater,groupNumber,profit);

	for(int i=0;i<groupNumber;i++){
		cout<<"第"<<i<<"组的盈利额为"<<profit[i];
		int number=0;
		for(int j=0;j<numQuater;j++){
		     if(MarketVSGroup[j][i]==1) number++;
		}
		cout<<" 跑赢市场的概率为："<<number*1.0/numQuater<<endl;
	}
	
	system("pause");
	
	return 0;

}

//input:每个季度的根据因子排好序的stockList 7*2000 的数组
//time:8个季度开始的时间，包含第0个季度的开始时间
//groupNumber：分组的数目
//output:每一个分组的利润和
//over
double szProfitRate=0.0;
int pec=0;
void computeProfit(THANDLE hTdb,vector<int> time,vector<vector<string>>stockList,vector <vector<bool>>& MarketVSGroup,int numQuater,int groupNumber,vector<double>& profit){
    vector<vector<double>>nums(numQuater,vector<double>(groupNumber,0.0)); //nums[季度][分组号]=某季度某分组的盈利和
	vector<vector<double>>numsRate(numQuater,vector<double>(groupNumber,0.0)); //numsRate[季度][分组号]=某季度某分组的收益率
	vector<double> marketTndexProfit(numQuater,0.0); //大盘的每个季度收益率
	
	for(int i=0;i<numQuater;i++){
		cout<<"第"<<i<<"季度正在计算"<<endl;
		cout<<stockList[i].size()<<endl;
		
		int startTime=time[i+1];
		int endTime=time[i];
		double szProfit=0.0;
		double szBaseProfit=0.0;
		//double szProfitRate=0.0;
		//getOneProfit(hTdb,startTime,endTime,"999999.sh",shProfit);//计算上证股市大盘该季度的盈利状况
		getOneProfit(hTdb,startTime,endTime,"399001.sz",szProfit,szBaseProfit);//计算深证股市大盘该季度的盈利状况
		szProfitRate=szProfit/szBaseProfit;
		//cout<<"上证综指"<<shProfit<<endl;
		cout<<"深证成指"<<szProfitRate<<endl;
		/*
		double baseProfit=0.0,oneProfit=0.0;
		for(int k=0;k<stockList[i].size();k++)
		     getOneProfit(hTdb,startTime,endTime,stockList[i][k],oneProfit,baseProfit);
		*/
		
		marketTndexProfit[i]=szProfitRate;
        int start=0;
		int k=0; //每个分组的开始位置
		for(int j=0;j<groupNumber;j++){
			   
			   int number=0;        
			   double baseProfit=0.0;  //每组的开始总资金
			   for(k=start;number<20 && k<stockList[i].size();k++){  //获取10只股票为1组
						 double oneProfit=0.0;  //单只股票的盈利
						 
						 if(getOneProfit(hTdb,startTime,endTime,stockList[i][k],oneProfit,baseProfit)){
							 nums[i][j]+=oneProfit;
							 number++;
						 }
			   }
			   cout<<i<<"季度"<<j<<"分组的股票数:"<<number<<" ";
			   start=k+100;     //每一个分组间隔100，确保各个分组的因子值差异比较大
			   cout<<nums[i][j]<<" / "<<baseProfit<<endl;     //分组的盈利/投资额
			   numsRate[i][j] = nums[i][j]/baseProfit; //该分组的年化收益率
			   cout<<numsRate[i][j]<<endl;
		}
		
		cout<<double(pec)/(20*groupNumber)<<endl;//计算单只股票跑赢市场的概率
		szProfitRate=0.0;
		pec=0;
		
	}
	cout<<"开始计算各个分组的盈利能力"<<endl;

	for(int i=0;i<numQuater;i++){
		for(int j=0;j<groupNumber;j++){
		     profit[j]+=nums[i][j];
			 if(numsRate[i][j]>marketTndexProfit[i])
				 MarketVSGroup[i][j]=true;
			 cout<<"季度："<<i<<"分组:"<<j<<"跑赢了市场："<<MarketVSGroup[i][j]<<endl;
		}
	}
}

//jop:将单支股票在 一段时间段内的盈利多少计算出来
//输入：时间段开始时间 时间段结束时间 股票代码
//输出：盈利额 如果TDB中不存在该股票信息则返回false
//over
bool getOneProfit(THANDLE hTdb,int startTime,int endTime,const string& stockid,double& oneProfit,double& baseProfit){
	TDBDefine_ReqKLine reqK = {"", REFILL_NONE, 0, 0, CYC_DAY, 1, 1, startTime, endTime, 0, 0};
	strncpy(reqK.chCode, stockid.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount =0;
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	if(nCount==0){
		//cout<<"未查询到该股票"<<stockid<<endl;
		return false;
	}
	//cout<<"查询到该股票"<<stockid<<endl;
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

//jop: 将csv文件数据导入，将股票代码格式转化为TDB格式
//输入：因子名称 factor,因子类型 S：static D:dynamic
//输出：股票列表 7*2000的数组 按照因子值排好序的stockid
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
		   //将stockid转化为TDB的格式
		    string TDBstockid=stockid;

           if(factorType=='S'){  //如果为静态类型则需要将股票代码转化为TDB的股票代码类型  下次再python中修改股票类型
		       string type=stockid.substr(0,2);
		       if(type=="sh")  //只取深圳股市股票数据
			        continue;
		       TDBstockid=stockid.substr(2,6)+'.'+type;
		   }

		   subStockList.push_back(TDBstockid);
	   }
	   stockList.push_back(subStockList);
	   ifs.close();    //记得关闭文件，否则无法读入下一个文件
	}
	for(int i=0;i<stockList.size();i++)
		cout<<"第"<<i+1<<"季度"<<stockList[i].size()<<endl;
}

//jop:获取测试集每个季度的开始时间，注意日期必须为交易日
//输入：
//输出：一个一维vector 存储第 i 个季度的开始日期
void getTestDate(vector<int>& testStartDate){
	testStartDate.push_back(20140901);    //0季度
	testStartDate.push_back(20140601);    //1季度
	testStartDate.push_back(20140301);
	testStartDate.push_back(20140101);
	testStartDate.push_back(20130901);
	testStartDate.push_back(20130601);
	testStartDate.push_back(20130301);
	testStartDate.push_back(20130101);

}
//获得一个完整交易日的日期
int getLastTradeDate(THANDLE hTdb,const std::string& strCode){
	time_t rawtime;
	struct tm * ptm;

	time(&rawtime);
	ptm = gmtime(&rawtime);

	//获得上一月25号到今天的所有日子
	int enddate=(ptm->tm_year+1900)*10000 + (ptm->tm_mon+1)*100 + ptm->tm_mday;
	int startdate=(ptm->tm_year+1900)*10000 + (ptm->tm_mon==0?12:ptm->tm_mon)*100 +25;

	TDBDefine_ReqKLine reqK = {"", REFILL_NONE, 0, 0, CYC_DAY, 1, 1, startdate, enddate, 0, 0};
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));

	TDBDefine_KLine* pKLine = NULL;
	int nCount =0;
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	TDBDefine_KLine& tdbK = *(pKLine + nCount-1); //计算最近的交易日日期，不管最后一个交易日是否完整
	int ResDate=tdbK.nDate;
	TDB_Free(pKLine);
	return ResDate;
}

//根据结束时间，时间间隔，计算开始时间
//输入：结束时间，时间间隔(月)
//输出：开始时间
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

		Print("---------------------收到%d项快照，错误码:%d -----------------\n", nCount,nRet);
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

		Print("---------------------收到%d项快照，错误码:%d -----------------\n", nCount, nRet);
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
//根据动态因子获取各个季度的股票排序列表
//input：数据库：htdb  时间段：testStartDate(7个季度) 股票列表7*2000（返回值):stockList
//output: void
void DynamicData(THANDLE hTdb,const string factor,vector<int> testStartDate,vector<string> stockList,vector<vector<string>>& rankStockList){
	_mkdir(factor.c_str());   //创建文件夹存储因子
	ofstream ofs;
	for(int i=0;i<7;i++){
		 ofs.open(factor+"/"+ to_string(long long (i+1))+".csv");
         vector<PAIR> stock_mom_pair;
		 int startDate=getStarDatetFromInterval(testStartDate[i],6);
		 for(int j=0;j<stockList.size();j++){
			  double mom;
			  //获取该股票n个月的动量  
		      if(getOneMomentum(hTdb,stockList[j],startDate,testStartDate[i],mom)==false)
				  continue;
			  PAIR a(stockList[j],mom);
			  stock_mom_pair.push_back(a);
		 }
		 cout<<"开始排序"<<endl;
		 sort(stock_mom_pair.begin(), stock_mom_pair.end(), CmpByValue());
		 cout<<"完成排序"<<endl;
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

//计算震荡指数
//最高价-最低价/开盘价+收盘价 =该段时间的震荡指数
//input: 数据库：htdb   股票代号：strCode  时间跨度：momCyc (1~12 月，从今天开始计算时间)  震荡指标（返回值）：oscillator
//output: void
void Oscillator(THANDLE hTdb, const std::string& strCode,int momCyc, double& oscillator){
	int nStartDay, nEndDay;
	nEndDay=getLastTradeDate(hTdb,strCode);
	nStartDay=nEndDay-10000;//一年的时间
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

//计算股票一年的波动率
//计算 log(每日收盘价/每日开盘价) 的平方差 * 一年交易日数量的平方根
//input:数据库:hTdb  股票代号:strCode  一年的波动率(此为返回值):volatility
//output:void
void Volatility(THANDLE hTdb, const std::string& strCode, double& volatility)
{
	int nStartDay, nEndDay;
	nEndDay=getLastTradeDate(hTdb,strCode);
	nStartDay=nEndDay-10000;//一年的时间
	TDBDefine_ReqKLine reqK = { "", REFILL_NONE, 0, 0, CYC_DAY,1,1, nStartDay, nEndDay,0,0 };
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount = 0;
	//TICK_BEGIN(minitue);1
	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);
	vector<double> tmp(nCount,0.0);
	//TICK_END(minitue);
	Print("---------------------收到%d项K线，错误码:%d -----------------\n", nCount, nRet);
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
	Print("---------------------收到%d项K线，错误码:%d -----------------\n", nCount, nRet);
	//ofstream outfile;
	//outfile.open("KLinefile.csv", ios::app);
	//outfile.open("GetK.csv");
	//outfile << "股票代码" << "," << "日期" << "," << "时间" << "," << "开盘价" << "," << "最高价" << "," <<"最低价" << "," << "收盘价" << "," <<"成交额" << "," << "iTurover" << endl;
	for (int i=0; i<nCount; i++) // 只会输出前10个
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
	Print("--------收到%d条逐笔成交，nRet:%d -------\n", nCount, nRet);
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
	Print("--------收到%d条逐笔委托，nRet:%d -------\n", nCount, nRet);
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
	Print("--------收到%d条委托队列，nRet:%d -------\n", nCount, nRet);
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


//上传下载计算股市公式
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
		//上载公式
		TDBDefine_AddFormulaRes resFormula = {0};
		int nRet = TDB_AddFormula(hTdb, arrFomula[i].szFormulaName, arrFomula[i].szFormulaText, &resFormula);
		printf("上传公式：nRet:%d, err line:%d, text:%s, info:%s\n", nRet, resFormula.nErrLine, resFormula.chText, resFormula.chInfo);
	}

	//获取已经上载的公式
	TDBDefine_FormulaItem* pFormula = NULL;
	int nFormulaCount = 0;
	int nRet = TDB_GetFormula(hTdb, "", &pFormula, &nFormulaCount);
	for (int i=0; i<nFormulaCount; i++)
	{
		printf("获取公式：nRet:%d, count:%d, name:%s, param:%s\n", nRet, nFormulaCount,pFormula[i].chFormulaName, pFormula[i].chParam);
	}

	//请求计算结果
	TDBDefine_CalcFormulaRes calcResult = {0};
	TDBDefine_ReqCalcFormula calcReq = {"","", "000001.sz", CYC_MINUTE, 1, REFILL_NONE, 0, 0, 4000, 400};
	//pFormula[0].chFormulaName,  pFormula[0].chParam
	const int nFormulaIndex = 3;

	//将calcReq 空缺的前两个字符串填上 分别是要计算公式名，需要的参数
	strncpy(calcReq.chFormulaName, pFormula[3].chFormulaName, sizeof(calcReq.chFormulaName));
	strncpy(calcReq.chParam, pFormula[3].chParam, sizeof(calcReq.chParam));

	nRet = TDB_CalcFormula(hTdb, &calcReq, &calcResult);

	printf("请求计算指标公式结果：nRet:%d, record count:%d, filed count:%d\n", nRet, calcResult.nRecordCount, calcResult.nFieldCount);

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

	//删除指标公式
	TDBDefine_DelFormulaRes pDelRes = {0};
	nRet = TDB_DeleteFormula(hTdb, "abc", &pDelRes);//
	printf("删除公式%s: nRet:%d, 信息：%s\n",  pDelRes.chFormularName,nRet, pDelRes.chInfo);
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

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
#include <vector>
#include <assert.h>
#include <string>
#include <time.h>
#include <fstream>
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

void Momentum(THANDLE hTdb, const std::string& strCode, int momCyc, int& monentum);
void Oscillator(THANDLE hTdb, const std::string& strCode,int momCyc, double& oscillator);
void Volatility(THANDLE hTdb, const std::string& strCode, double& volatility);

void LoadIPFromCin(OPEN_SETTINGS&);

int getLastTradeDate(THANDLE hTdb,const std::string& strCode);
double getOneProfit(THANDLE hTdb,string startTime,string endTime,string stockid);
void getData(string factor,vector<vector<string>>& stockList);
void getTestDate(vector<string>& testStartDate);
void computeProfit(vector<vector<string>>stockList,vector<double> profit);
int main(int argc, char* argv[])
{   
	OPEN_SETTINGS settings={"127.0.0.1","10222","user","pwd",30,2,0}; // 登录时的信息,并赋予初值
	LoadIPFromCin(settings); // 输入登录信息
	TDBDefine_ResLogin loginAnswer={0}; 
	/*
	THANDLE hTdb = TDB_Open(&settings, &loginAnswer);
	if (!hTdb)
	{ 
		Print("TDB_Open failed:%s, program exit!\n", loginAnswer.szInfo);
		exit(0);
	}
	*/
	string strCode="601857.sh";
	int monentum=0;
	double oscillator=0.0;
	double volatility=0.0;
	//Momentum(hTdb,strCode,12,monentum);
	//Oscillator(hTdb, strCode,1, oscillator);
	//Volatility(hTdb, strCode, volatility);
	
	vector<double> profit;
	vector<vector<string>>stockList;
	string factor="EPS";
	vector<string> testStartDate; 
	getTestDate(testStartDate);
	getData(factor,stockList);
	//computeProfit(hTdb,stocklist,profit);
	return 0;

}

//input:每个季度的根据因子排好序的stockList 7*2000 的数组
//time:8个季度开始的时间，包含第0个季度的开始时间
//output:每一个分组的利润和
void computeProfit(THANDLE hTdb,vector<string> time,vector<vector<string>>stockList,vector<double> profit){
    vector<vector<double>>nums; //nums[季度][分组号]=某季度某分组的盈利和
	
	for(int i=0;i<7;i++){
        int start=0;
		string startTime=time[i+1];
		string endTime=time[i];
		for(int j=0;j<5;j++){
           for(int k=start;k<start+400;k++){
		         nums[i][j]+=getOneProfit(hTdb,startTime,endTime,stockList[i][k]);
		   }
		   start+=400;
		}
	}

	for(int i=0;i<7;i++){
		for(int j=0;j<5;j++){
		     profit[j]+=nums[i][j];
		}
	}
}


double getOneProfit(THANDLE hTdb,string startTime,string endTime,string stockid){
	return 0.0;
}

//jop: 将csv文件数据导入，将股票代码格式转化为TDB格式
//输入：因子类型
//输出：股票列表 7*2000的数组 按照因子值排好序的stockid
void getData(string factor,vector<vector<string>>& stockList){
    ifstream ifs;
	for(int i=0;i<7;i++){
	   ifs.open(factor+"/"+to_string(long long (i))+".csv");
	   string data;
	   while(getline(ifs,data)){
	       cout<<data<<endl;
	   }
	}
}

//jop:获取测试集每个季度的开始时间，注意日期必须为交易日
//输入：
//输出：一个一维vector 存储第 i 个季度的开始日期
void getTestDate(vector<string>& testStartDate){
	testStartDate.push_back("20140901");    //0季度
	testStartDate.push_back("20140603");    //1季度
	testStartDate.push_back("20140303");
	testStartDate.push_back("20140102");
	testStartDate.push_back("20130902");
	testStartDate.push_back("20130603");
	testStartDate.push_back("20130301");
	testStartDate.push_back("20130104");

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

//计算动量，momCyc为6月或者12月
//今日收盘价-当日的开盘价 = 一段时间的动量
//input：数据库：htdb  股票代号：strCode 时间段：momCyc(1~12 月，从今天开始计算时间) 动量（返回值):monentum
//output: void
void Momentum(THANDLE hTdb, const std::string& strCode,int momCyc,int& monentum){
	int nStartDay, nEndDay;
	nEndDay=getLastTradeDate(hTdb,strCode);
	nStartDay=nEndDay-10000;//一年的时间
	TDBDefine_ReqKLine reqK = { "", REFILL_NONE, 0, 0, CYC_MONTH,1,1, nStartDay, nEndDay,0,0 };
	//TDBDefine_ReqKLine reqK = {"", REFILL_NONE, 0, 0, CYC_DAY, 1, 1, startdate, enddate, 0, 0};
	strncpy(reqK.chCode, strCode.c_str(), sizeof(reqK.chCode));
	TDBDefine_KLine* pKLine = NULL;
	int nCount = 0;

	int nRet = TDB_GetKLine(hTdb, &reqK, &pKLine, &nCount);

	Print("---------------------收到%d项K线，错误码:%d -----------------\n", nCount, nRet);
	for (int i=0; i<nCount; i++) // 只会输出前10个
	{
		TDBDefine_KLine& tdbK = *(pKLine+i);
		cout<<tdbK.nDate<<endl;
	}
	int openLast = (pKLine+ 12 - momCyc)->nOpen; //根据6月还是12月 选择开始月份作为开盘价
	int closeToday = (pKLine+nCount-1)->nClose; //获取最后一个月的收盘价
	cout<<openLast<<","<<closeToday;
	monentum = closeToday - openLast;
	system("pause");
	TDB_Free(pKLine);
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

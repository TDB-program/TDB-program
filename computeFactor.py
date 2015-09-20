#coding=utf-8
from pymongo import MongoClient
import pymongo
import csv
import os

#compute the factor PE (市盈率)
#input mongodb info
#output the stock list ranked by the value of factor
def computePE(FundaInfo):
    factorName="EPS"
    for i in range(1,8):                 # 1季度 -- 7季度   最近第0季度预留准备测试因子
        stock_factor_dict=dict()
        dataSet=FundaInfo.find({"quarter":i})
        for document in dataSet:
            if  document[factorName]=="0":
                document[factorName]=fillZero(FundaInfo,document['stockid'],factorName,i)
            if  document["ClosePrice"]=="0":
                document["ClosePrice"]=fillZero(FundaInfo,document['stockid'],"ClosePrice",i)
            PE=round(float(document['ClosePrice'])/float(document[factorName]),2)    #round() 只取小数后三位
            stock_factor_dict[document['stockid']]=PE

        print len(stock_factor_dict)
        stock_factor_dict= sorted(stock_factor_dict.iteritems(), key=lambda d:d[1], reverse = True)  #对因子进行排序
        if os.path.exists(factorName)==False:                          #mkdir save the factor into the associated dir
             os.mkdir(factorName)
        csvfile = file(factorName+'/'+str(i)+'.csv', 'wb')
        #csvfile = file(str(i)+'quarterEPS.csv', 'wb')
        writer = csv.writer(csvfile)
        for key,val in stock_factor_dict:
            writer.writerow([key,val])
        csvfile.close()

def fillZero(FundaInfo,stockid,col,quarter):    #返回最近季度非零的值
     dataSet=FundaInfo.find({"stockid":stockid,"quarter":{"$gt":quarter-2} })
     #print len(dataSet)
     for data in dataSet:
         print data
         if data[col]!="":
             FundaInfo.update({"stockid":stockid,"quarter":quarter} ,{col:data[col]})
             return data[col]

if __name__ == '__main__':
    # for i in range(0,7):
    #     print i
    client=MongoClient("127.0.0.1",27017)                  #connect to mongo
    db=client.TDB                                            #connect to database
    FundaInfo=db.FundaInfo                                   #connect to collections    factor value
    computePE(FundaInfo)

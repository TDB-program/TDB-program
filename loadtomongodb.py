# -*- coding:utf-8 -*-
from pymongo import MongoClient
import csv

def saveToMongo():
   client=MongoClient("127.0.0.1",27017)                  #connect to mongo9
   db=client.TDB                                            #connect to database
   LabelInfo=db.LabelInfo                                   #connect to collection     factor real name <==> factor variable name
   LabelInfo.drop()
   FundaInfo=db.FundaInfo                                   #connect to collections    factor value
   FundaInfo.drop()
   csvfile = file('fundaFactor.csv', 'rb')                  #read the file
   reader=csv.reader(csvfile)
   i=0
   for line in reader:                                     #line is list
        i=i+1
        if i==1:
            LabelInfo.insert({"stockid":line[0].decode("GBK"),"name":line[1].decode("GBK"),"quarter":line[2].decode("GBK"),"ClosePrice":line[3].decode("GBK"),"EPS":line[4].decode("GBK"),"totalAssets":line[5].decode("GBK"),"gLiability":line[6].decode("GBK"),
                          "totalValue":line[7].decode("GBK"),"fAssets":line[8].decode("GBK"),"cMValue":line[9].decode("GBK"),"taking":line[10].decode("GBK"),"rProfits":line[11].decode("GBK"),"oProfit":line[12].decode("GBK"),"oCosts":line[13].decode("GBK"),
                          "ROE":line[14].decode("GBK")})
            continue
        timeList=line[2].split('/');
        Quarter=(2014-int(timeList[2]))*4 + (12-int(timeList[0]))/3   #计算季度
        if line[3]=="": line[3]="0"                                  #将空位填充为0字符串
        if line[4]=="": line[4]="0"
        if line[5]=="": line[5]="0"
        if line[6]=="": line[6]="0"
        if line[7]=="": line[7]="0"
        if line[8]=="": line[8]="0"
        if line[9]=="": line[9]="0"
        if line[10]=="": line[10]="0"
        if line[11]=="": line[11]="0"
        if line[12]=="": line[12]="0"
        if line[13]=="": line[13]="0"
        if line[14]=="": line[14]="0"

        FundaInfo.insert({"stockid":line[0],"name":line[1].decode("GBK"),"quarter":Quarter,"ClosePrice":line[3],"EPS":line[4],"totalAssets":line[5],"gLiability":line[6],
                          "totalValue":line[7],"fAssets":line[8],"cMValue":line[9],"taking":line[10],"rProfits":line[11],"oProfit":line[12],"oCosts":line[13],
                          "ROE":line[14]})


   csvfile.close()
if __name__ == '__main__':
    saveToMongo()


#中文乱码问题
#python代码最顶端需要加上 # -*- coding:utf-8 -*- 声明该源文件的中文编码方式
#中文的那个字符串需要加上 .decode("GBK") 以GBK编码对字符串str进行解码，以获取unicode 就可以了
#more:http://blog.csdn.net/moodytong/article/details/8136258
 FundaInfo.update({"stockid":stockid,"quarter":quarter} ,{$set:{col:dataSet[col]}})
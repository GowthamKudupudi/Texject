/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TestTxj.cpp
 * Author: gowtham
 *
 * Created on 26 August, 2016, 10:49 PM
 */

#include <stdlib.h>
#include <iostream>
#include <logger.h>
#include <ferrybase/FerryTimeStamp.h>
#include <ferrybase/mystdlib.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <ios>
#include <random>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "Texject.h"

typedef const char* ccp;

/*
 * Simple C++ Test Suite
 */
using namespace std;

#include <ferrybase/ThreadPool.h>

thread_local int tid= 0;
ThreadPool* tpoolPtr= nullptr;

int child_exit_status = 0;
FF_LOG_TYPE fflAllowedType = (FF_LOG_TYPE)(FFL_DEBUG | FFL_INFO | FFL_ERR);
unsigned int fflAllowedBlks = 9|TXJ_MAIN;
FerryTimeStamp ftsStart;
FerryTimeStamp ftsEnd;
FerryTimeStamp ftsDiff;

double vm, rss, initrss= 0;
void mem_usage(double& vm_usage, double& resident_set) {
   vm_usage = 0.0;
   resident_set = 0.0;
   //get info from proc directory
   ifstream stat_stream("/proc/self/stat",ios_base::in);
   //create some variables to get info
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;
   unsigned long vsize;
   long rss;
   // don't care about the rest
   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
   >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
   >> utime >> stime >> cutime >> cstime >> priority >> nice
   >> O >> itrealvalue >> starttime >> vsize >> rss; 
   stat_stream.close();
   // for x86-64 is configured to use 2MB pages
   long page_size_kib = sysconf(_SC_PAGE_SIZE) / 1024; 
   vm_usage = vsize / 1024.0;
   resident_set = rss * page_size_kib;
}

void printMemUsage () {
   mem_usage(vm, rss);
   //cout<< "Virtual Memory: "<< vm<< "KiB"<< endl;
	cout<< "rss: "<< rss<< ", arss: "<< rss-initrss<< "KiB"<< endl;
	initrss= rss;
}

struct Test_ {
   Test_ () {
      cout << "TestConstructor" << endl;
   };
   ~Test_ () {
      cout << "TestDestructor" << endl;
   };
};
Test_ testFunc1 () {
   Test_ t;
   return t;
}
Test_ testFunc2 () {
   Test_ t;
   return t;
}
void test1 () {
   cout << "===================================================" << endl;
   cout << "               TestTxj test 1                      " << endl;
   cout << "===================================================" << endl;
   Txj_ f(Txj_::ARRAY);
   f[0]=1;
   char cCurrentPath[FILENAME_MAX];
   
   if (!GetCurrentDir(cCurrentPath, sizeof (cCurrentPath))) {
      return;
   }
   
   cCurrentPath[sizeof (cCurrentPath) - 1] = '\0'; /* not really required */
   char c = '\r';
   ffl_info(1, "\\r=%d", (int) c);
   ffl_info(1, "The current working directory is %s", cCurrentPath);
   fflush(stdout);
   string fn = "sample.txj";
   //string fn = "/home/gowtham/Projects/ferrymediaserver/output.txj";
   ifstream ifs(fn.c_str(), ios::in | ios::ate);
   string ffjsonStr;
   ifs.seekg(0, ios::end);
   ffjsonStr.reserve(ifs.tellg());
   ifs.seekg(0, ios::beg);
   ffjsonStr.assign((istreambuf_iterator<char>(ifs)),
                    istreambuf_iterator<char>());
   Txj_ ffo(ffjsonStr);
   cout << "amphibians: " << endl;
   Txj_::Iterator i = ffo["amphibians"].begin(); //["amphibians"]
   while ((i != ffo["amphibians"].end())) {
      cout << string(i) << ":" << i->stringify() << endl;
      ++i;
   }
   cout << endl;
   string ps = ffo.prettyString(false, true);
   cout << ps << endl;
   Txj_ ffo2(ps);
   ffo2["amphibians"]["genome"].setEFlag(Txj_::E_FLAGS::B64ENCODE);
   ffo2["amphibians"]["salamanders"] = "salee";
   string ps2 = ffo2.prettyString(false, true);
   cout << ps2 << endl;
   ffo2["amphibians"]["salamanders"] = "malee";
   ffo2["amphibians"]["count"] = 4;
   ffo2["amphibians"]["density"] = 5.6;
   ffo2["amphibians"]["count"] = 5;
   ffo2["amphibians"]["genome"] = "<xml>sadfalejhjroifndk</xml>";
   if (ffo2["amphibians"]["gowtham"]) {
      cout << ffo2["amphibians"].size << endl;
      ffo2["amphibians"].trim();
      cout << ffo2["amphibians"].size << endl;
   };
   ffo2["animals"][3] = "satish";
   cout << ffo2["animals"][4].prettyString() << endl;
   cout << "size: " << ffo2["animals"].size << endl;
   ffo2["animals"][3] = "bear";
   cout << "after bear inserted at 4" << ffo2["animals"].prettyString() << endl;
   cout << "size: " << ffo2["animals"].size << endl;
   ffo2["animals"].trim();
   cout << "size after trim: " << ffo2["animals"].size << endl;
   string ps3 = ffo2.prettyString();
   cout << ps3 << endl;
   cout << "Txj_ signature size: " << sizeof (ffo2) << endl;
   
   cout << "sizeInfo test 1" << endl;
   
   cout << "size of char: " << sizeof (char) << endl;
   cout << "size of short: " << sizeof (short) << endl;
   cout << "size of int: " << sizeof (int) << endl;
   cout << "size of long: " << sizeof (long) << endl;
   cout << "size of long long: " << sizeof (long long) << endl;
   
   cout << "size of float: " << sizeof (float) << endl;
   cout << "size of double: " << sizeof (double) << endl;
   
   cout << "size of pointer: " << sizeof (int *) << endl;
   
   ffo2["amphibians"]["frogs"].setQType(Txj_::QUERY_TYPE::QUERY);
   ffo2["amphibians"]["salamanders"].setQType(Txj_::QUERY_TYPE::DEL);
   ffo2["amphibians"]["genome"].setQType(Txj_::QUERY_TYPE::SET);
   ffo2["birds"][1].setQType(Txj_::QUERY_TYPE::DEL);
   ffo2["birds"][2].setQType(Txj_::QUERY_TYPE::SET);
   ffo2["birds"][3].setQType(Txj_::QUERY_TYPE::QUERY);
   string query = ffo2.queryString();
   ffo2["amphibians"]["genome"]= "<xml>gnomechanged :p</xml>";
   ffo2["birds"][2]= "kiwi";
   cout << ffo2.prettyString() << endl;
   cout << query << endl;
   Txj_ qo(query);
   query = qo.queryString();
   cout<< query<< endl;
   
   if (ffo2["amphibians"]["frogs"].isEFlagSet(Txj_::E_FLAGS::EXTENDED)) {
      cout << "already extended" << endl;
   }
   Txj_* ao = ffo2.answerObject(&qo);
   if (ffo2["amphibians"]["frogs"].isEFlagSet(Txj_::E_FLAGS::EXTENDED)) {
      cout << "already extended" << endl;
   }
   
   cout << ao->stringify() << endl;
   string ffo2a = ffo2.prettyString();
   cout << ffo2a << endl;
   Txj_ ffo2ao(ffo2a);
   ffo2a = ffo2ao.stringify();
   cout << ffo2a << endl;
   ffo2a = ffo2ao.prettyString();
   cout << ffo2a << endl;
   delete ao;
   cout << "3rd students Maths marks: " <<
   ffo2["studentsMarks"][2]["Maths"].prettyString() << endl;
   if (ffo2["null"] == nullptr){
      cout << "ffo2[\"null\"] == NULL" << endl;
   }
   cout << "end of test" << endl;
   return;
}

struct testStruct {
   string* s;
};

void test2 () {
   cout << "===================================================" << endl;
   cout << "               TestTxj test 2                   " << endl;
   cout << "===================================================" << endl;

   string fn = "/home/gowtham/Projects/ferrymediaserver/offpmpack.json";
   ifstream ifs(fn.c_str(), ios::in | ios::ate);
   if (ifs.is_open()) {
      string ffjsonStr;
      ifs.seekg(0, ios::end);
      ffjsonStr.reserve(ifs.tellg());
      ifs.seekg(0, ios::beg);
      ffjsonStr.assign((istreambuf_iterator<char>(ifs)),
                       istreambuf_iterator<char>());
      Txj_ ffo(ffjsonStr);
      ffo["ferryframes"].setEFlag(Txj_::B64ENCODE);
      string* s = new string(ffo.stringify(true));
      cout << *s << endl;
      s->append(":)");
      testStruct ts;
      ts.s = s;
      delete ts.s;
   }
   cout << "%TEST_PASSED%" << endl;
}

void test3 () {
   cout << "===================================================" << endl;
   cout << "        TestTxj_ test 3 (comparing strings)      " << endl;
   cout << "===================================================" << endl;
   
   Txj_ sample("file://sample.txj");
   if ((int) sample["donkeys"] < 4) {
      cout << "alert: my donkey is missing" << endl;
   }
   if (strcmp("John", sample["example"]["employees"][0]["firstName"]) == 0) {
      cout << "info: yes John is fist employee!" << endl;
   }
   cout << "%TEST_PASSED%" << endl;
}

void test4 () {
   cout << "===================================================" << endl;
   cout << "			TestTxj_ test 4 (testing links)		   " << endl;
   cout << "===================================================" << endl;
   Txj_ f("file://linksSample.txj");
   map<string,Txj_*>* emln = f["obj1"].val.pairs;
   typedef const char* ccp;
   if (emln->find(string("127.0.0.2"))!=emln->end()) {
      Txj_* ffemln = (*emln)["127.0.0.2"];
      Txj_::Link* link =
         ffemln->getFeaturedMember(Txj_::FM_LINK).link;
      const char* linkName=(*link)[0].c_str();
      cout << "127.0.0.2 is link to " << linkName << endl;
   }
   cout << (const char*)f["obj1"]["127.0.0.2"]["rootdir"] << endl;
   cout << f << endl;
   cout << f["things"]["car"][0] << endl;
   Txj_& ff = f["things"]["car"][1];
   ff.addLink(f, "users.gowtham.things.1");
   cout << f << endl;
   cout << "%TEST_PASSED%" << endl;
}

void test5 () {
   cout << "===================================================" << endl;
   cout << "		TestTxj test 5 (testing extensions)		   " << endl;
   cout << "===================================================" << endl;
   Txj_ f("file://ExtensionTest.txj");
   cout << f.prettyString() << endl;
   
   cout << "Marks[0]['Maths']: " << f["Marks"][0]["Maths"].prettyString()
   << endl;
   
   cout << "StudentsMarks['Gowtham']['Maths']: "
   << f["School"]["Class1"]["StudentsMarks"]["Gowtham"]["Maths"].prettyString()
   << endl;
   Txj_ f2(f.prettyString());
   cout << f2.prettyString() << endl;
   
   Txj_ f3(f2);
   cout << "f3 StudentsMarks['Gowtham']['Maths']: "
   << f3["School"]["Class1"]["StudentsMarks"]["Gowtham"]["Maths"].prettyString()
   << endl;
   Txj_ f4(f2.stringify());
   cout << f4.stringify() << endl;
}

void test6 () {
   cout << "===================================================" << endl;
   cout << "	TestTxj test 6 (testing data type sizes)		" << endl;
   cout << "===================================================" << endl;
   map<string, Txj_*> m;
   pair<string, Txj_*> p(string("gowtham"), (Txj_*) NULL);
   cout << &p.first << endl;
   m.insert(p);
   cout << &(*m.find("gowtham")) << endl;
   cout << &(*m.find("gowtham")) << endl;
   int i;
   Txj_ f;
   vector<string*> v;
   map<string, Txj_*>::iterator it;
   cout << "map:" << sizeof (m) << endl;
   cout << "int:" << sizeof (i) << endl;
   cout << "ffjson:" << sizeof (f) << endl;
   cout << "vector:" << sizeof (v) << endl;
   cout << "iterator:" << sizeof (it) << endl;
   cout << "FerryTimeStamp:" << sizeof (FerryTimeStamp) << endl;
   cout << "FerryTimeStamp&:" << sizeof (FerryTimeStamp&) << endl;
   cout << "double:" << sizeof (double) << endl;
   
}

void test7 () {
   cout << "===================================================" << endl;
   cout << "	TestTxj test 7 (testing MultiLineArray)		" << endl;
   cout << "===================================================" << endl;
   Txj_ f("file://MultiLineArray.txj");
   string sF = f.prettyString();
   cout << sF << endl;
   Txj_ f2(sF);
   string sF2 = f2.stringify();
   cout << f2 << endl;
   Txj_ f3(sF2);
   string sF3 = f3.prettyString();
   cout << sF3 << endl;
   Txj_ f4(sF3);
   string sF4 = f4.stringify();
   cout << sF4 << endl;
   Txj_ f5(sF4);
   string sF5 = f5.prettyString();
   cout << sF5 << endl;
}

void test8 () {
   cout << "===================================================" << endl;
   cout << "           TestTxj test 8 sample.txj               " << endl;
   cout << "===================================================" << endl;
   //Txj_ f("file://example.json");
   //Txj_ f("file://Employee.oob.txj");
   //Txj_ f("file://red/users2.obj.txj");
   Txj_ f("{\"Rs\":{\"20\":[2]}}");
	int i= f["Rs"]["20"][0];
   //Txj f("[\n]");
   cout<< f.prettyString()<< endl;
   cout<< f.stringify()<< endl;
   // Txj_ f2(f.prettyString());
   // cout<< f2<< endl;
   // string sF2= f2.stringify();
   // cout<< sF2<< endl;
   // Txj_ f3(sF2);
   // string sF3= f3.prettyString();
   // cout<< sF3<< endl;
	//f["necktwi"]["things"][0]["id"]= 1728;
	//f.save();
}

void test9 () {
   cout << "===================================================" << endl;
   cout << "                     erase test                    " << endl;
   cout << "===================================================" << endl;
   Txj_ f("{}");
   f["cameras"].erase(string("cam"));
   
}

struct MyStruct {
   int tv_sec;
   int tv_nsec;
};

void test10 () {
   cout << "===================================================" << endl;
   cout << "				 typecast   test					" << endl;
   cout << "===================================================" << endl;
   Txj_ f("{}");
   f["a"] = *(new timespec());
   timespec& t = (timespec&) f["a"];
   Txj_& ff = f;
   timespec& tt = (timespec&) ff["a"];
   tt.tv_sec = 'a';
   tt.tv_nsec = 'b';
   cout << ff << endl;
   cout << "parsing string" << endl;
   Txj_ f3(ff.prettyString());
   cout << f3 << endl;
   timespec& t3 = (timespec&) f3["a"];
   cout << (char) t3.tv_sec << "," << (char) t3.tv_nsec << endl;
}

void test11 () {
   cout << "===================================================" << endl;
   cout << "      subscript operator exection flow	            " << endl;
   cout << "===================================================" << endl;
   Txj_ f("{}");
   f["a"]["b"] = 2;
   Txj_& b = f["a"]["b"];
   int bb = (int) f["a"]["b"];
   if (f["b"]) {
      cout << "itWontPrint" << endl;
   }
}

void test12 () {
   cout << "===================================================" << endl;
   cout << "               update query                        " << endl;
   cout << "===================================================" << endl;
   Txj_ f("{necktwi:{things:[{id:0}]}}");
   int j = 10;
   //while (j) {
      cout << "Creating new answer object: " << endl;
      Txj_ tf("{necktwi:{things:[{name:\"batman\"}]}}");
      Txj_ tf2("{necktwi:{things:[{name:?}]}}");
      static FerryTimeStamp ft;
      Txj_ ao(f);
      Txj_* ff = f.answerObject(&tf,NULL,ft, &ao);
      //ft.update();
      if(!ff)return;
      cout << "res: " << *ff << endl;
      //delete ff;
      ff = f.answerObject(&tf2,NULL,ft);
      cout << *ff << endl;
      //f["newState"] = "RECORD";
      //j--;
      //}
}

void test13 () {
   cout << "===================================================" << endl;
   cout << "                       save file					      " << endl;
   cout << "===================================================" << endl;
   Txj_ fa(Txj_::ARRAY);
   fa[0]=1;
   Txj_ f("file://saveFileSample.txj|OBJECT");
   f["test"]="OK";
   //f["obj4"]["nestedFile"]["test"]="OK";
   Txj_ pvh;
   pvh = &f["vh"]["obj6"];
   pvh["users"]["test"]="OK";
   f["txoTest"]["test"]="OK";
   f["txoTest"].clearEFlag(Txj_::FILE);
   cout << f << endl;
   f.save();
   // Txj_ ff("file://saveFileSample.txj");
   // Txj_& ln = ff["vh"]["obj5"]["things"][].
   //    addLink(ff["vh"]["obj5"], "users.gowtham.things.0");
   // if (!ln)
   //    delete &ln;
   // cout << ff << endl;
   // ff.save();
}

void test14 () {
   cout << "===================================================" << endl;
   cout << "                       leak test					      " << endl;
   cout << "===================================================" << endl;
   Txj_ f("file:///home/Necktwi/workspace/ferryfair/config.txj");
   cout << f << endl;
}

void test15 () {
   Txj_ f("file:///home/Necktwi/workspace/ferryfair/config.txj");
   cout << f << endl;
}

vector<string> strs;
void test16 () {
   for (int i=0; i<1000000; ++i)
      strs.push_back(random_alphnuma_string(24));
   sort(strs.begin(), strs.end());
}

void test17 () {
   string randstr = random_alphnuma_string(24);
   auto it = lower_bound(strs.begin(), strs.end(), randstr);
   strs.insert(it, randstr);
}

void test18 () {
   cout << "===================================================" << endl;
   cout << "                       set test					      " << endl;
   cout << "===================================================" << endl;
   Txj_ f("{1, 2, 3, 3}");
   cout << f << endl;
}

void test19 () {
   cout << "===================================================" << endl;
   cout << "                       Test_  					      " << endl;
   cout << "===================================================" << endl;
   Test_ t1 = testFunc1();
   //Test_& t2 = testFunc1();
   Test_ t3 = testFunc1();
   //Test_& t4 = testFunc1();
}

Txj_ t;
void test20 () {
   cout << "===================================================" << endl;
   cout << "                       StressTest				      " << endl;
   cout << "===================================================" << endl;
   for (int i=0; i<1; ++i) {
      string a("a");
      a+=to_string(i);
      for (int j=0; j<1000;++j) {
         for (int k=0; k<10;++k) {
            string c = "c";
            c += to_string(k);
            string val = a +"b" + to_string(j) + c;
            t[a][j][c]=val;
            //cout << val;
         }
      }
      cout << ".";fflush(stdout);
   }
   double vm, rss;
   mem_usage(vm, rss);
   double tt = 3.6789787878*pow(10,6);
   printf("%3.0lf", tt);
   cout << "Virtual Memory: " << vm << "\nResident set size: " << rss << endl;
}

char* returnCharDeleteStr () {
   string s("movable");
   return const_cast<char*>(std::move(s).c_str());
}

void test22 () {
   char* cs;
   {
      string s("movable");
      //cs = returnCharDeleteStr(std::move(s));
      //cs = const_cast<char*>(std::move(s).c_str());
      cs = returnCharDeleteStr();
      cout << s << endl;
   }
   string s2("movable2");
   string s3("movable3");
   string sa[100];
   for (int i=0;i<100;++i) {
      sa[i]=to_string(i)+"imovable";
      cout << sa[i] <<endl;
   }
   cout << s2 << endl;
   cout << s3 << endl;
   cs[0]='L';
   cout << cs << endl;
   //free(cs);
}

int test24 () {
	cout << "## 1. int copy test" << endl;
	int i= 1;
	string istr= to_string(i);
	ccp cstr= istr.c_str();
   ftsStart.update();
	Txj_ t(cstr);
	Txj_ t2(t);
	Txj_ t3;
	t3= t2;
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< t<< endl;
   cout<< t2<< endl;
   cout<< "i: "<< i<< endl;
   cout<< "t3: "<< t3<< endl;
	printMemUsage();
	cout<< "Testing: t3==i"<< endl;
	if ((int)t3==i) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test25 () {
	cout << "## 2. string copy test" << endl;
	ccp tstr= "somestring";
	string str("\"");
	str+= tstr; str+= "\"";
	ccp cstr= str.c_str();
   ftsStart.update();
	Txj_ t(cstr);
	Txj_ t2(t);
	Txj_ t3;
	t3= t2;
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< t<< endl;
   cout<< t2<< endl;
   cout<< "tstr: "<< tstr<< endl;
   cout<< "(ccp)t3: "<< (ccp)t3<< endl;
	printMemUsage();
	cout<< "Testing: (ccp)t3==tstr"<< endl;
	if (!strcmp((ccp)t3, tstr)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test26 () {
	cout << "## 3. obj stringify test" << endl;
	ccp istr= "{i:1,s:\"somestring\"}";
	ccp jstr= "{\"i\":1,\"s\":\"somestring\"}";
	ftsStart.update();
	Txj_ t(istr);
   Txj_ tj(jstr);
	string ostr= t.stringify();
	string ojstr= tj.stringify(true);
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "istr: "<< istr<< endl;
	cout<< "jstr: "<< jstr<< endl;
	cout<< "ostr: "<< ostr<< endl;
	cout<< "ojstr: "<< ojstr<< endl;
	printMemUsage();
	cout<< "Testing: ostr==istr && ojstr==jstr"<< endl;
	if (!strcmp(ostr.c_str(), istr) && !strcmp(ojstr.c_str(), jstr)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test27 () {
	cout << "## 4. obj copy test" << endl;
	ccp istr= "{i:1,s:\"somestring\"}";
	ftsStart.update();
	Txj_ t(istr);
	Txj_ t2(t);
	Txj_ t3;
	t3= t2;
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "istr: "<< istr<< endl;
	cout<< t<< endl;
   cout<< t2<< endl;
	string ostr= t3.stringify();
	cout<< "ostr: "<< ostr<< endl;
	printMemUsage();
	cout<< "Testing: ostr==istr"<< endl;
	if (!strcmp(ostr.c_str(), istr)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test28 () {
	cout << "## 5. array copy test" << endl;
	ccp istr= "[1,\"somestring\"]";
	ftsStart.update();
	Txj_ t(istr);
	Txj_ t2(t);
	Txj_ t3;
	t3= t2;
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "istr: "<< istr<< endl;
	cout<< t<< endl;
   cout<< t2<< endl;
	string ostr= t3.stringify();
	cout<< "ostr: "<< ostr<< endl;
	printMemUsage();
	cout<< "Testing: ostr==istr"<< endl;
	if (!strcmp(ostr.c_str(), istr)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test29 () {
	cout << "## 6. url string test" << endl;
	ccp istr= "file://tests/data/urlstring.obj.txj";
	ftsStart.update();
	Txj_ t(istr);
	ifstream ifs("tests/data/urlstring.obj.txj", ios::in);
	string fStr("{");
	if (ifs.is_open()) {
		fStr.append((istreambuf_iterator<char>(ifs)),
						  istreambuf_iterator<char>());
		ifs.close();
	}
	fStr+="}";
	string ostr= t.stringify();
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "fstr: "<< fStr<< endl;
	cout<< "ostr: "<< ostr<< endl;
	printMemUsage();
	cout<< "Testing: ostr==fStr"<< endl;
	if (ostr==fStr) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test30 () {
	cout << "## 7. parse example.json test" << endl;
	ccp istr= "file://tests/data/simple.json";
	ftsStart.update();
	Txj_ t(istr);
	string ostr= t.stringify();
	bool areContractors= t["areContractors"];
	ccp firstEmployeeFirstName= t["employees"][0]["firstName"];
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "areContractors: "<< areContractors<< endl;
	cout<< "firsEmployeeFirstName: "<< firstEmployeeFirstName<< endl;
	cout<< "ostr: "<< ostr<< endl;
	printMemUsage();
	cout<< "Testing: areContractors==true && firstEmployeeFirstName==John"<<
		endl;
	if (areContractors && !strcmp(firstEmployeeFirstName, "John")) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test31 () {
	cout << "## 8. parse nullmember.obj.txj test" << endl;
	ccp istr= "file://tests/data/nullmember.obj.txj";
	ftsStart.update();
	Txj_ t(istr);
	string ostr= t.stringify();
	Txj_& rbsid= t["2W2tzZxC0ufJ05Xp"];
	ccp user= rbsid["user"];
	Txj_& wpSub= rbsid["wpSub"];
	Txj_& keys= wpSub["keys"];
	ccp endpoint= wpSub["endpoint"];
	ccp p256dh= keys["p256dh"], authKe= keys["auth"];
	bool wpSubNull= !wpSub;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "user: "<< user<< endl;
	cout<< "ostr: "<< ostr<< endl;
	printMemUsage();
	cout<< "Testing: user==gowtham && !wpSub && !endpoint && !p256dh &&"
		" !authKe"<< endl;
	if (user && !strcmp(user, "gowtham") && wpSubNull &&
		 !endpoint && !p256dh && !authKe) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test32 () {
	cout << "## 9. Entended array and multiline string test" << endl;
	ccp istr= "file://tests/data/Employee.oob.txj";
	ccp expectedBio= "He is smart, brilliant, genius, empathetic, creative, "
		"connective, patient,\nhandsome, valient, romantic;) After all he made "
		"JSON with multi line string!";
	ftsStart.update();
	Txj_ t(istr);
	string ostr= t.stringify();
	string name= (ccp)t["name"];
	int cppScore= t["testScore"][0]["C++"];
	int jsScore= t["testScore"][0]["Javascript"];
	int lispScore= t["testScore"][0]["lisp"];
	t["testScore"][0]["lisp"]= lispScore+1;
	string biography= (ccp)t["biography"];
	t.save();
	t.init(istr);
	int lispScore2= t["testScore"][0]["lisp"];
	t["testScore"][0]["lisp"]= lispScore2-1;
	t.save();
	t.init(istr);
	int lispScore3= t["testScore"][0]["lisp"];
	t.save();
	t["sports"][]= "tt"; //its already in set, so no effect! 
	cout<< "Sports he play: "<< t["sports"]<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "name: "<< name<< endl;
	cout<< "biography: "<< biography<< endl;
	cout<< "expectedBio: "<< expectedBio<< endl;
	cout<< "cppScore: "<< cppScore<< endl;
	cout<< "jsScore: "<< jsScore<< endl;
	cout<< "lispScore: "<< lispScore<< endl;
	cout<< "ostr: "<< ostr<< endl;
	cout<< "lispScore2: "<< lispScore2<< endl;
	cout<< "lispScore3: "<< lispScore3<< endl;
	
	printMemUsage();
	cout<< "Testing: name==Gowtham && lispScore==7 && lispScore==8 && "
		"lispScore3==7 && biography==expectedBio"<< endl;
	if (!name.empty() && !strcmp(name.c_str(), "Gowtham") && lispScore==6 &&
		 lispScore2==7 && lispScore3==6 &&
		 !strcmp(biography.c_str(), expectedBio)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test33 () {
	cout<< "## 10. one line create large array test"<< endl;
	ftsStart.update();
	Txj_ t;
	t["a1"][667]["c8"]= 1;
   cout<< "t[\"a1\"][666][\"c8\"]:"<< t["a1"][666]["c8"]<< endl;
	cout<< "t[\"a1\"][666][\"c8\"]:"<< t["a1"][667]["c8"]<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: !t[\"a1\"][666][\"c8\"] &&"
		" t[\"a1\"][667][\"c8\"]==1"<< endl;
	if (!t["a1"][666]["c8"] && (int)t["a1"][667]["c8"]==1) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test34 () {
	cout<< "## 11. empty string, obj and array test"<< endl;
	ccp cobj= "{}", carr= "[]", cvoid= "";
	ftsStart.update();
	Txj_ otxj(cobj), atxj(carr), vtxj(cvoid);
	string ostr= otxj.prettyString();
	string astr= atxj.prettyString();
	string vstr= vtxj.prettyString();
	cout<< "ostr: "<< ostr<< endl;
	cout<< "astr: "<< astr<< endl;
	cout<< "vstr: "<< vstr<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: ostr=='{}' && astr=='[]' && vstr==\"\""<< endl;
	if (!strcmp(ostr.c_str(), cobj) && !strcmp(astr.c_str(), carr) &&
		 !strcmp(vstr.c_str(), cvoid)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test35 () {
	cout<< "## 12. char[] test" <<endl;
	char un[48]= "gowtham";
	ftsStart.update();
 	Txj_ fun;
	fun= (ccp)un;
	ccp cfun= (ccp)fun;
	ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	cout<< "un: "<< un<< endl;
	cout<< "cfun: "<< cfun<< endl;
	printMemUsage();
	cout<< "Testing: un==cfun"<< endl;
	if (!strcmp(un, cfun)) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test36 () {
	cout<< "## 13. table test"<< endl;
	ccp istr= "file://tests/data/table.oob.txj";
	ftsStart.update();
	Txj_ t(istr);
	string ostr= t.prettyString();
	cout<< "ostr: "<< ostr<< endl;
	int sal= t["EmployeeDetails"][0]["Salary"];
	cout<< "t[\"EmployeeDetails\"][0][\"Salary\"]"<< sal<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: t[\"EmployeeDetails\"][0][\"Salary\"]==5"<< endl;
	if (sal==5) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test37 () {
	cout<< "## 14. nested files links copy stringify test"<< endl;
	ccp istr= "file://tests/data/red/users2.obj.txj";
	ftsStart.update();
	Txj_ r, t(istr);
	Txj_& rt= r["necktwi"]["things"][0];
	rt["id"]= t["necktwi"]["things"][0]["id"];
	rt["user"]= &t["necktwi"]["things"][0]["user"]["name"];
	string ostr= t.prettyString();
	cout<< "ostr: "<< ostr<< endl;
	string ostr2; rt.stringify(ostr2, true);
	cout<< "ostr2: "<< ostr2<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: rt[\"user\"]==Necktwi"<< endl;
	if (!strcmp(rt["user"], "Necktwi")) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test38 () {
	cout<< "## 15. comments test"<< endl;
	ccp istr= "file://tests/data/comment.oob.txj";
	ftsStart.update();
	Txj_ t(istr);
	string prettyTxj= t.prettyString();
	cout<< "prettyTxj: "<< prettyTxj<< endl;
	t.save();
	cout<< "istr+7: "<< istr+7<< endl;
	ifstream ifs(istr+7, ios::in);
	string fStr;
	if (ifs.is_open()) {
		fStr.append((istreambuf_iterator<char>(ifs)),
						  istreambuf_iterator<char>());
		ifs.close();
		cout<< "fStr: "<< endl<< fStr<< endl;
	}
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: '#comment' should not exist in prettyTxj but should exist in fStr"<< endl;
	if (!strstr(prettyTxj.c_str(), "#comment") &&
		 strstr(fStr.c_str(), "#comment")) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test39 () {
	cout<< "## 16. timestamp test"<< endl;
	ftsStart.update();
	ccp istr= "file://tests/data/empty.obj.txj";
	Txj_ t(istr);
	t.freeObj();
	auto now= chrono::system_clock::now();
	t["qC2VvW3jIYeMVg30"]["ts"]= now;
	string prettyTxj= t.prettyString(false, true);
	cout<< "prettyTxj: "<< prettyTxj<< endl;
	now= chrono::system_clock::now();
	t["qC2VvW3jIYeMVg30"]["ts"]= now;
	prettyTxj= t.prettyString(false, true);
	cout<< "prettyTxj: "<< prettyTxj<< endl;
	t.save();
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: prettyTxj has (8)"<< endl;
	if (strstr(prettyTxj.c_str(), "(8)")) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test40 () {
	cout<< "## 13. table copy test"<< endl;
	ccp istr= "file://tests/data/table.oob.txj";
	ftsStart.update();
	Txj_ t(istr);
	Txj_ c(t);
	string ostr= c.prettyString();
	cout<< "ostr: "<< ostr<< endl;
	int sal= c["EmployeeDetails"][0]["Salary"];
	cout<< "c[\"EmployeeDetails\"][0][\"Salary\"]: "<< sal<< endl;
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: c[\"EmployeeDetails\"][0][\"Salary\"]==5"<< endl;
	if (sal==5) {
		cout<< "PASSED"<< endl<< endl;
		return 1;
	} else {
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
}

int test41 () {
	cout<< "## 17. malformed Texject crash test"<< endl;
	ftsStart.update();
	const char* cases[]= {
		"",
		"   \n\t  ",
		"\n",
		"1",
		"null",
		"nul",
		"true",
		"tru",
		"false",
		"!",
		"?",
		"^",
		"delete",
		"del",
		"1 2",
		"a: 1",
		"a: 1,",
		"a: 1,,",
		"{}",
		"[]",
		"{} extra",
		"}",
		"]",
		",",
		"#",
		"#comment",
		"#comment\na: 1",
		"{a: 1}",
		"{a: 1,}",
		"{,}",
		"{,a: 1}",
		"{a: 1,,b: 2}",
		"{a: 1,,}",
		"{a: 1, b: 2,,}",
		"{a: 1,, b: 2}",
		"{a: , b: 2}",
		"{a: }",
		"{a: ,}",
		"[,]",
		"[,1]",
		"[1,]",
		"[1,,2]",
		"[1,,]",
		"[, ,]",
		"{a: 1",
		"[1",
		"{a: {b: 1",
		"{a: [1, 2",
		"{a: 1} }",
		"[1] ]",
		"{a: 1}\n{b: 2}",
		"{a: 1} garbage",
		"{a: ()}",
		"{a: (0)xyz}",
		"{a: (3)xy}",
		"{a: (3)xy, b: 2}",
		"{a: (9999)xy}",
		"{a: (9999)xy, b: 2}",
		"{a: (3)",
		"{a: (3",
		"{a: (-1)}",
		"{a: (1)5, b: (1)x}",
		"(3)abc",
		"{a: \"unterminated}",
		"{a: \"unterminated",
		"{a: \"x\"}",
		"{a: \"multi\nline\", b: 2}",
		"{a: \"x\" \"y\"}",
		"{a: \"x\" \"y\", b: 2}",
		"{a: \"\"}",
		"{a: 1.2.3}",
		"{a: 1.2, b: 3}",
		"{a: .5}",
		"{a: 5.}",
		"{a: 1e10}",
		"{a: 0x10}",
		"{a: -}",
		"{a: +}",
		"{a: --1}",
		"{a: 20260101120000123456}",
		"{a: 2026010112000012345}",
		"{a: 202601011200001234567}",
		"{#c, a: 1}",
		"{a: 1, #c}",
		"{a: 1, #c, b: 2}",
		"{#}",
		"{a: 1, #}",
		"{\n#comment\na: 1\n}",
		"{a: 1, #comment line\n, b: 2}",
		"{a: [1, 2,]}",
		"{a: [1, 2,]",
		"{{}}",
		"[[]]",
		"{a: {}}",
		"{a: []}",
		"{}{}",
		"[{} {}]",
		"{a: 1, b: {c: 2,}} extra",
		"{\"a\": 1}",
		"{a: 1, \"a\": 2}",
		"{1: 2}",
		"{1.5: 2}",
		"{-1: 2}",
		"{+1: 2}",
		"{a: (Time)}",
		"{a: 1, b: 2} #trailing",
		"{a: .b}",
		"{a: .b, b: 1}",
	};
	int n= (int)(sizeof(cases)/sizeof(cases[0]));
	int crashes= 0;
	for (int c= 58; c<n; ++c) {
		cout<< "case "<< c<< ": `"<< cases[c]<< "`"<< endl;
		pid_t pid= fork();
		if (pid==0) {
			Txj_ t;
			t.init(cases[c]);
			cout<< "  init ok"<< endl;
			string s;
			t.prettyString(s, false, true);
			cout<< "  prettyString ok"<< endl;
			Txj_ r;
			r.init(s);
			cout<< "  re-init ok"<< endl;
			_exit(0);
		}
		int st= 0;
		waitpid(pid, &st, 0);
		if (!WIFEXITED(st) || WEXITSTATUS(st)!=0) {
			++crashes;
			cout<< "  *** CRASHED *** status=0x"<< hex<< st<< dec<< endl;
			return 0;
		}
	}
	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "%TEST_FINISHED% in "<< ftsDiff<< "sec"<< endl;
	printMemUsage();
	cout<< "Testing: "<< n<<
		 " malformed Texjects init/prettyString/re-init without crash"<< endl;
	if (crashes) {
		cout<< "crashes: "<< crashes<< endl;
		cout<< "FAILED"<< endl<< endl;
		return 0;
	}
	cout<< "PASSED"<< endl<< endl;
	return 1;
}

// ---------- test41: thread-safety stress test ----------
static Txj_ t41Root (Txj_::OBJ);
static atomic<int64_t> t41TotalOps {0};
static atomic<uint64_t> t41KeyCtr {0};
static const uint64_t t41_TARGET= 100000;
static const int t41_MAX_MEMBERS= 7;
static const int t41_NUM_THREADS= 4;

thread_local random_device t41_rd;
thread_local mt19937_64 t41_rng{t41_rd()};

static bool t41_isCtnr (Txj_& t) {
	return t.isType(Txj_::OBJ) || t.isType(Txj_::ARRAY) ||
		t.isType(Txj_::ORDERED_OBJ) || t.isType(Txj_::SET_TYPE);
}
static bool noContainer (Txj_* c) {
	if (!t41_isCtnr(*c))
		return true;
	Txj_::Iterator it= c->begin();
	Txj_::Iterator end= c->end();
	for (; it!=end; ++it) {
		if (t41_isCtnr(*it))
			return false;
	}
	return true;
}

static Txj_* t41_makeLeaf () {
	uniform_int_distribution<int> dist(0, 4);
	int t= dist(t41_rng);
	Txj_* p= new Txj_();
	switch (t) {
	case 0: *p= string("hello world"); break;
	case 1: *p= (int)42; break;
	case 2: *p= 3.14159; break;
	case 3: *p= true; break;
	case 4: {
		uint8_t* d= (uint8_t*)malloc(8);
		d[0]=1; d[1]=2; d[2]=3; d[3]=4;
		d[4]=5; d[5]=6; d[6]=7; d[7]=8;
		*p= Txj_::Blob_{d, 8};
		break;}
	}
	return p;
}
static Txj_* t41_insertInto (Txj_& c, Txj_& child);
static Txj_* t41_makeSimpleContainer () {
	uniform_int_distribution<int> dist(0, 3);
	int t= dist(t41_rng);
	Txj_* p= nullptr;
	switch (t) {
	case 0: p= new Txj_(Txj_::OBJ);
		(*p)["seed"]= 42;
		break;
	case 1: p= new Txj_(Txj_::ARRAY);
		(*p)[0]= 42;
		break;
	case 2: p= new Txj_(Txj_::ORDERED_OBJ);
		(*p)["seed"]= 42;
		break;
	default: p= new Txj_();
		(*p)[]= 42;
		(*p)[]= 43;
		break;
	}
	return p;
}
static Txj_* t41_makeContainer () {
	uniform_int_distribution<int> dist(0, 3);
	int t= dist(t41_rng);
	Txj_* p= nullptr;
	if (t==3) {
		p= new Txj_();
		(*p)[]= 100;
		(*p)[]= 200;
		(*p)[]= 300;
		return p;
	}
	switch (t) {
	case 0: p= new Txj_(Txj_::OBJ);
		break;
	case 1: p= new Txj_(Txj_::ARRAY);
		break;
	default: p= new Txj_(Txj_::ORDERED_OBJ);
		break;
	}
	Txj_* child= t41_makeSimpleContainer();
	t41_insertInto(*p, *child);
	delete child;
	return p;
}
static Txj_* t41_insertInto (Txj_& c, Txj_& child) {
	c.lockShared();
	if (c.size >= (unsigned)t41_MAX_MEMBERS) {
		c.unlockShared();
		return nullptr;
	}
	bool isArr= c.isType(Txj_::ARRAY);
	bool isSet= c.isType(Txj_::SET_TYPE);
	unsigned asz= isArr? c.size: 0;
	c.unlockShared();
	if (isSet) {
		// c[] locks c internally, so call it WITHOUT holding c's lock;
		// the set-insert inside operator= is not locked, so take c's
		// exclusive lock only around `mem= child`
		Txj_& mem= c[];
		Txj_* pp= &mem;
		c.lock();
		unsigned sz= c.size;
		if (sz >= (unsigned)t41_MAX_MEMBERS) {
			c.unlock();
			delete pp;
			return nullptr;
		}
		mem= child;
		bool ok= c.size==sz+1;
		c.unlock();
		// do NOT delete pp here: on the dedup path operator= has already
		// deleted the proxy (it `delete this`s when set insert fails);
		// on success pp is a live set member
		return ok? pp : nullptr;
	}
	string k= "k" + to_string(t41KeyCtr.fetch_add(1));
	Txj_& mem= isArr? c[asz] : c[k];
	mem= child;
	return &mem;
}
static Txj_* t41_pickTxj (Txj_& c, bool container) {
	if (c.size == 0)
		return container? &c : nullptr;
	uniform_int_distribution<int> d(0, c.size);
	int skip;
  randgen:
	skip= d(t41_rng);
	if (skip==c.size) {
		if (container)
			return &c;
		else
			goto randgen;
	}
	uniform_int_distribution<int> d2(0, 1);
	switch (c.getType()) {
	case Txj_::ARRAY: {
		uniform_int_distribution<int> d(0, c.size-1);
		Txj_& m= (*c)[d(t41_rng)];
		if (container && !t41_isCtnr(m))
			return nullptr;
		return d2(t41_rng)? t41_pickTxj(m, true):&m;
	}
	case Txj_::OBJ:
	case Txj_::ORDERED_OBJ:
	case Txj_::SET_TYPE: {
		Txj_::Iterator it= c.begin();
		Txj_::Iterator end= c.end();
		for (int i= 0; i<skip && it!=end; ++it,++i) {}
		if (it==end)
			return nullptr;
		Txj_& m= *it;
		if (container && !t41_isCtnr(m))
			return nullptr;
		return d2(t41_rng)? t41_pickTxj(m, true):&m;
	}}
	return nullptr;
}
static void t41_eraseMember (Txj_& c, Txj_& mem) {
	if (c.isType(Txj_::ARRAY)) {
		Txj_::Iterator it= c.begin();
		Txj_::Iterator end= c.end();
		int idx= 0;
		while (it!=end) {
			if (&(*it)==&mem) {
				c.erase(idx);
				return;
			}
			++it;
			++idx;
		}
	} else if (c.isType(Txj_::OBJ) || c.isType(Txj_::ORDERED_OBJ)) {
		Txj_::Iterator it= c.begin();
		Txj_::Iterator end= c.end();
		while (it!=end) {
			if (&(*it)==&mem) {
				string key= it.getIndex();
				c.erase(key);
				return;
			}
			++it;
		}
	}
}
static const char* t41_tn (Txj_& t) {
	if (t.isType(Txj_::OBJ)) return "obj";
	if (t.isType(Txj_::ARRAY)) return "arr";
	if (t.isType(Txj_::ORDERED_OBJ)) return "oob";
	if (t.isType(Txj_::SET_TYPE)) return "set";
	if (t.isType(Txj_::STRING)) return "str";
	if (t.isType(Txj_::NUMBER)) return "num";
	if (t.isType(Txj_::BOOL)) return "bool";
	if (t.isType(Txj_::BINARY)) return "bin";
	if (t.isType(Txj_::TIME)) return "time";
	static char buf[16];
	snprintf(buf, sizeof(buf), "t%d", (int)t.getType());
	return buf;
}
static uint64_t t41_countNodes (Txj_& t) {
	if (!t41_isCtnr(t))
		return 1;
	t.lockShared();
	uint64_t n= 1;
	if (t.isType(Txj_::ARRAY)) {
		for (unsigned i=0; i<t.size; ++i) {
			Txj_& m= (*t)[i];
			n+= t41_isCtnr(m)? t41_countNodes(m): 1;
		}
	} else {
		Txj_::Iterator it= t.begin();
		Txj_::Iterator end= t.end();
		for (; it!=end; ++it) {
			Txj_& m= *it;
			n+= t41_isCtnr(m)? t41_countNodes(m): 1;
		}
	}
	t.unlockShared();
	return n;
}
static void t41_workerBody (int);
static void t41_worker (int) {
	try {
		t41_workerBody(tid);
	} catch (const exception& e) {
		void* frames[64];
		int n= backtrace(frames, 64);
		char** strs= backtrace_symbols(frames, n);
		printf("tid:%d worker exception: %s\n", tid, e.what());
		for (int i=0; i<n; ++i)
			printf("  %s\n", strs[i]);
		throw;
	}
}
static void t41_workerBody (int) {
	while (t41TotalOps.load() < t41_TARGET) {
		uniform_int_distribution<int> ad(0, 4);
		int a= ad(t41_rng);
		Txj_* c= t41_pickTxj(t41Root, true);
		if (!c) continue;
		if (c->size==6 && noContainer(c)) {
			Txj_* nc= t41_makeContainer();
			printf("tid:%d inserting (%lld)%p<%s> in %p<%s>\n",
					 tid, (long long)t41TotalOps.load(), nc, t41_tn(*nc),
					 c, t41_tn(*c));
			Txj_* inserted= t41_insertInto(*c, *nc);
			if (inserted)
				t41TotalOps.fetch_add(1);
			delete nc;
			continue;
		}
		if (a <= 1) {
			if (!c)
				continue;
			uniform_int_distribution<int> td(0, 3);
			if (td(t41_rng) <= 1) {
				Txj_* leaf= t41_makeLeaf();
				printf("tid:%d inserting (%lld)%p<%s> in %p<%s>\n",
					tid, (long long)t41TotalOps.load(), leaf,
					t41_tn(*leaf), c, t41_tn(*c));
				if (t41_insertInto(*c, *leaf))
					t41TotalOps.fetch_add(1);
				delete leaf;
			} else {
				Txj_* nc= t41_makeContainer();
				printf("tid:%d inserting (%lld)%p<%s> in %p<%s>\n",
					tid, (long long)t41TotalOps.load(), nc,
					t41_tn(*nc), c, t41_tn(*c));
				Txj_* inserted= t41_insertInto(*c, *nc);
				if (inserted)
					t41TotalOps.fetch_add(1);
				delete nc;
			}
		} else if (a <= 2) {
			Txj_* src= t41_pickTxj(t41Root, true);
			if (!src)
				continue;
			if (!src->tryLockShared()) {
				continue;
			}
			Txj_* srcMem= t41_pickTxj(*src, false);
			src->unlockShared();
			if (!srcMem)
				continue;
			uint64_t cnt= 1;
			if (t41_isCtnr(*srcMem)) {
				cnt= t41_countNodes(*srcMem);
				if (cnt-1 > 7)
					continue;
			}
			if (!c)
				continue;
			printf("tid:%d copying (%lld)%p<%s> from %p<%s> to %p<%s>\n",
				 tid, (long long)t41TotalOps.load(), srcMem,
				 t41_tn(*srcMem), src, t41_tn(*src), c,
				 t41_tn(*c));
			Txj_ tmp(*srcMem);
			if (t41_insertInto(*c, tmp))
				t41TotalOps.fetch_add(cnt);
		} else {
			if (!c || c->size < 2)
				continue;
			switch (c->getType()) {
			case Txj_::OBJ:
			case Txj_::ORDERED_OBJ:
			case Txj_::SET_TYPE: {
				Txj_::Iterator it= c->begin();
				Txj_::Iterator end= c->end();
				uniform_int_distribution<int> d(0, c->size-1);
				int skip= d(t41_rng);
				for (int i=0; i<skip && it!=end; ++it,++i) {}
				if (it!=end) {
					Txj_& mem= *it;
					switch (mem.getType()) {
					case Txj_::OBJ:
					case Txj_::ORDERED_OBJ:
					case Txj_::SET_TYPE: {
						printf("tid:%d deleting %p<%s> from %p<%s>\n",
								 tid, &mem, t41_tn(mem), c, t41_tn(*c));
						if (c->isType(Txj_::SET_TYPE))
							c->erase(&mem);
						else
							c->erase(it.getIndex());
						--t41TotalOps;}
					}
				}}
			}
		}
	}
}
int test42 () {
	cout << "## 42. thread safety stress test" << endl;
	t41TotalOps.store(0);
	t41KeyCtr.store(0);

	cout << "  " << t41_NUM_THREADS << " threads, "
	     << t41_TARGET << " ops" << endl;
	ftsStart.update();

	ThreadPool tp(t41_NUM_THREADS);
	for (int i=0; i<t41_NUM_THREADS; ++i)
		tp.enqueue(t41_worker);
	tp.join();

	ftsEnd.update();
	ftsDiff= ftsEnd-ftsStart;
	cout<< "  done in "<< ftsDiff<< "sec"<< endl;
	t41Root.freeObj();
	cout<< "PASSED"<< endl<< endl;
	return 1;
}

int main (int argc, char** argv) {
	cout<< "%SUITE_STARTING% TestTxj"<< endl;
	cout<< "%SUITE_STARTED%"<< endl;

	FerryTimeStamp ftsSuiteStart;
	FerryTimeStamp ftsSuiteEnd;
	ftsSuiteStart.update();

/*
   cout << "%TEST_STARTED% test1 (TestTxj)" << endl;
   ftsStart.update();
   test1();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff <<
      " test1 (TestTxj)" << endl;

   cout << "%TEST_STARTED% test2 (TestTxj)\n" << endl;
   ftsStart.update();
   test2();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test2 (TestTxj)" << endl;
   
   cout << "%TEST_STARTED% test3 (TestTxj)\n" << endl;
   ftsStart.update();
   test3();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test3 " << endl;

   cout << "%TEST_STARTED% test4 (TestTxj)\n" << endl;
   ftsStart.update();
   test4();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test4 " << endl;

   cout << "%TEST_STARTED% test5 (TestTxj)\n" << endl;
   ftsStart.update();
   test5();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test5 " << endl;
   
   cout << "%TEST_STARTED% test6 (TestTxj)\n" << endl;
   ftsStart.update();
   test6();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test6 " << endl;

   cout << "%TEST_STARTED% test7 (TestTxj)\n" << endl;
   ftsStart.update();
   test7();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test7 " << endl;

   cout << "%TEST_STARTED% test8 (TestTxj)\n" << endl;
   ftsStart.update();
   test8();
   ftsEnd.update();
   ftsDiff= ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test8 " << endl;

   cout << "%TEST_STARTED% test9\n" << endl;
   ftsStart.update();
   test9();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test9 " << endl;
   
   cout << "%TEST_STARTED% test10\n" << endl;
   ftsStart.update();
   test10();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test10 " << endl;

   cout << "%TEST_STARTED% test11\n" << endl;
   ftsStart.update();
   test11();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test11 " << endl;

   cout << "%TEST_STARTED% test12\n" << endl;
   ftsStart.update();
   test12();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test12 " << endl;

   cout << "%TEST_STARTED% test13\n" << endl;
   ftsStart.update();
   test13();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test13 " << endl;

   cout << "%TEST_STARTED% test15\n" << endl;
   ftsStart.update();
   test15();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test15 " << endl;

   cout << "%TEST_STARTED% test16" << endl;
   ftsStart.update();
   test16();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test16\n" << endl;

   cout << "%TEST_STARTED% test17" << endl;
   ftsStart.update();
   test17();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test17\n" << endl;

   cout << "%TEST_STARTED% test18" << endl;
   ftsStart.update();
   test18();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test18\n" << endl;

   cout << "%TEST_STARTED% test19" << endl;
   ftsStart.update();
   test19();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test19\n" << endl;

   cout << "%TEST_STARTED% test20" << endl;
   ftsStart.update();
   test20();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test20\n" << endl;
  
   cout << "%TEST_STARTED% test21" << endl;
   ftsStart.update();
   test21();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test21\n" << endl;

   cout << "%TEST_STARTED% test22" << endl;
   ftsStart.update();
   test22();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test22" << endl;
*/
	int pc= 0, tc=0;
	printMemUsage(); cout<< endl;

	++tc; pc+= test24();
	++tc; pc+= test25();
	++tc; pc+= test26();
	++tc; pc+= test27();
	++tc; pc+= test28();
	++tc; pc+= test29();
	++tc; pc+= test30();
	++tc; pc+= test31();
	++tc; pc+= test32();
	++tc; pc+= test33();
	++tc; pc+= test34();
	++tc; pc+= test35();
	++tc; pc+= test36();
	++tc; pc+= test37();
	++tc; pc+= test38();
	++tc; pc+= test39();
	++tc; pc+= test40();
	++tc; pc+= test41();
	++tc; pc+= test42();

	ftsSuiteEnd.update();
   ftsDiff= ftsSuiteEnd-ftsSuiteStart;
   cout<< "%SUITE_FINISHED% time="<< ftsDiff<< "sec"<< endl;
	cout<< "TotalPassed: "<< pc<< "/"<< tc<< endl;

   return (EXIT_SUCCESS);
}

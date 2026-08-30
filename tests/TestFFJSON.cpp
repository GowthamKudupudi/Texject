/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TestFFJSON.cpp
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
#include <unistd.h>
#include <math.h>
#include <ios>
#include "FFJSON.h"

typedef const char* ccp;

/*
 * Simple C++ Test Suite
 */
using namespace std;

int child_exit_status = 0;
FF_LOG_TYPE fflAllowedType = (FF_LOG_TYPE)(FFL_DEBUG | FFL_INFO);
unsigned int fflAllowedBlks = 9;

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
   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; 
   vm_usage = vsize / 1024.0;
   resident_set = rss * page_size_kb;
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
   cout << "               TestFFJSON test 1                   " << endl;
   cout << "===================================================" << endl;
   FFJSON f(FFJSON::ARRAY);
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
   string fn = "sample.ffjson";
   //string fn = "/home/gowtham/Projects/ferrymediaserver/output.ffjson";
   ifstream ifs(fn.c_str(), ios::in | ios::ate);
   string ffjsonStr;
   ifs.seekg(0, ios::end);
   ffjsonStr.reserve(ifs.tellg());
   ifs.seekg(0, ios::beg);
   ffjsonStr.assign((istreambuf_iterator<char>(ifs)),
                    istreambuf_iterator<char>());
   FFJSON ffo(ffjsonStr);
   cout << "amphibians: " << endl;
   FFJSON::Iterator i = ffo["amphibians"].begin(); //["amphibians"]
   while ((i != ffo["amphibians"].end())) {
      cout << string(i) << ":" << i->stringify() << endl;
      ++i;
   }
   cout << endl;
   string ps = ffo.prettyString(false, true);
   cout << ps << endl;
   FFJSON ffo2(ps);
   ffo2["amphibians"]["genome"].setEFlag(FFJSON::E_FLAGS::B64ENCODE);
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
   cout << "FFJSON signature size: " << sizeof (ffo2) << endl;
   
   cout << "sizeInfo test 1" << endl;
   
   cout << "size of char: " << sizeof (char) << endl;
   cout << "size of short: " << sizeof (short) << endl;
   cout << "size of int: " << sizeof (int) << endl;
   cout << "size of long: " << sizeof (long) << endl;
   cout << "size of long long: " << sizeof (long long) << endl;
   
   cout << "size of float: " << sizeof (float) << endl;
   cout << "size of double: " << sizeof (double) << endl;
   
   cout << "size of pointer: " << sizeof (int *) << endl;
   
   ffo2["amphibians"]["frogs"].setQType(FFJSON::QUERY_TYPE::QUERY);
   ffo2["amphibians"]["salamanders"].setQType(FFJSON::QUERY_TYPE::DEL);
   ffo2["amphibians"]["genome"].setQType(FFJSON::QUERY_TYPE::SET);
   ffo2["birds"][1].setQType(FFJSON::QUERY_TYPE::DEL);
   ffo2["birds"][2].setQType(FFJSON::QUERY_TYPE::SET);
   ffo2["birds"][3].setQType(FFJSON::QUERY_TYPE::QUERY);
   string query = ffo2.queryString();
   ffo2["amphibians"]["genome"] = "<xml>gnomechanged :p</xml>";
   ffo2["birds"][2] = "kiwi";
   cout << ffo2.prettyString() << endl;
   cout << query << endl;
   FFJSON qo(query);
   query = qo.queryString();
   cout << query << endl;
   
   if (ffo2["amphibians"]["frogs"].isEFlagSet(FFJSON::E_FLAGS::EXTENDED)) {
      cout << "already extended" << endl;
   }
   FFJSON* ao = ffo2.answerObject(&qo);
   if (ffo2["amphibians"]["frogs"].isEFlagSet(FFJSON::E_FLAGS::EXTENDED)) {
      cout << "already extended" << endl;
   }
   
   cout << ao->stringify() << endl;
   string ffo2a = ffo2.prettyString();
   cout << ffo2a << endl;
   FFJSON ffo2ao(ffo2a);
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
   cout << "               TestFFJSON test 2                   " << endl;
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
      FFJSON ffo(ffjsonStr);
      ffo["ferryframes"].setEFlag(FFJSON::B64ENCODE);
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
   cout << "        TestFFJSON test 3 (comparing strings)      " << endl;
   cout << "===================================================" << endl;
   
   FFJSON sample("file://sample.ffjson");
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
   cout << "			TestFFJSON test 4 (testing links)		   " << endl;
   cout << "===================================================" << endl;
   FFJSON f("file://linksSample.ffjson");
   map<string,FFJSON*>* emln = f["obj1"].val.pairs;
   typedef const char* ccp;
   if (emln->find(string("127.0.0.2"))!=emln->end()) {
      FFJSON* ffemln = (*emln)["127.0.0.2"];
      FFJSON::Link* link =
         ffemln->getFeaturedMember(FFJSON::FM_LINK).link;
      const char* linkName=(*link)[0].c_str();
      cout << "127.0.0.2 is link to " << linkName << endl;
   }
   cout << (const char*)f["obj1"]["127.0.0.2"]["rootdir"] << endl;
   cout << f << endl;
   cout << f["things"]["car"][0] << endl;
   FFJSON& ff = f["things"]["car"][1];
   ff.addLink(f, "users.gowtham.things.1");
   cout << f << endl;
   cout << "%TEST_PASSED%" << endl;
}

void test5 () {
   cout << "===================================================" << endl;
   cout << "		TestFFJSON test 5 (testing extensions)		   " << endl;
   cout << "===================================================" << endl;
   FFJSON f("file://ExtensionTest.ffjson");
   cout << f.prettyString() << endl;
   
   cout << "Marks[0]['Maths']: " << f["Marks"][0]["Maths"].prettyString()
   << endl;
   
   cout << "StudentsMarks['Gowtham']['Maths']: "
   << f["School"]["Class1"]["StudentsMarks"]["Gowtham"]["Maths"].prettyString()
   << endl;
   FFJSON f2(f.prettyString());
   cout << f2.prettyString() << endl;
   
   FFJSON f3(f2);
   cout << "f3 StudentsMarks['Gowtham']['Maths']: "
   << f3["School"]["Class1"]["StudentsMarks"]["Gowtham"]["Maths"].prettyString()
   << endl;
   FFJSON f4(f2.stringify());
   cout << f4.stringify() << endl;
}

void test6 () {
   cout << "===================================================" << endl;
   cout << "	TestFFJSON test 6 (testing data type sizes)		" << endl;
   cout << "===================================================" << endl;
   map<string, FFJSON*> m;
   pair<string, FFJSON*> p(string("gowtham"), (FFJSON*) NULL);
   cout << &p.first << endl;
   m.insert(p);
   cout << &(*m.find("gowtham")) << endl;
   cout << &(*m.find("gowtham")) << endl;
   int i;
   FFJSON f;
   vector<string*> v;
   map<string, FFJSON*>::iterator it;
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
   cout << "	TestFFJSON test 7 (testing MultiLineArray)		" << endl;
   cout << "===================================================" << endl;
   FFJSON f("file://MultiLineArray.ffjson");
   string sF = f.prettyString();
   cout << sF << endl;
   FFJSON f2(sF);
   string sF2 = f2.stringify();
   cout << f2 << endl;
   FFJSON f3(sF2);
   string sF3 = f3.prettyString();
   cout << sF3 << endl;
   FFJSON f4(sF3);
   string sF4 = f4.stringify();
   cout << sF4 << endl;
   FFJSON f5(sF4);
   string sF5 = f5.prettyString();
   cout << sF5 << endl;
}

void test8 () {
   cout << "===================================================" << endl;
   cout << "           TestFFJSON test 8 sample.ffjson         " << endl;
   cout << "===================================================" << endl;
   FFJSON f("file://example.json");
   cout << f.prettyString() << endl;
   FFJSON f2(f.prettyString());
   string sF2 = f2.stringify();
   cout << f2 << endl;
   FFJSON f3(sF2);
   string sF3 = f3.prettyString();
   cout << sF3 << endl;
   
}

void test9 () {
   cout << "===================================================" << endl;
   cout << "						erase test					" << endl;
   cout << "===================================================" << endl;
   FFJSON f("{}");
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
   FFJSON f("{}");
   f["a"] = *(new timespec());
   timespec& t = (timespec&) f["a"];
   FFJSON& ff = f;
   timespec& tt = (timespec&) ff["a"];
   tt.tv_sec = 'a';
   tt.tv_nsec = 'b';
   cout << ff << endl;
   cout << "parsing string" << endl;
   FFJSON f3(ff.prettyString());
   cout << f3 << endl;
   timespec& t3 = (timespec&) f3["a"];
   cout << (char) t3.tv_sec << "," << (char) t3.tv_nsec << endl;
}

void test11 () {
   cout << "===================================================" << endl;
   cout << "		subscript operator exection flow			" << endl;
   cout << "===================================================" << endl;
   FFJSON f("{}");
   f["a"]["b"] = 2;
   FFJSON& b = f["a"]["b"];
   int bb = (int) f["a"]["b"];
   if (f["b"]) {
      cout << "itWontPrint" << endl;
   }
}

void test12 () {
   cout << "===================================================" << endl;
   cout << "					update query					" << endl;
   cout << "===================================================" << endl;
   FFJSON f("{necktwi:{things:[{id:0}]}}");
   int j = 10;
   //while (j) {
      cout << "Creating new answer object: " << endl;
      FFJSON tf("{necktwi:{things:[{name:\"batman\"}]}}");
      FFJSON tf2("{necktwi:{things:[{name:?}]}}");
      static FerryTimeStamp ft;
      FFJSON ao(f);
      FFJSON* ff = f.answerObject(&tf,NULL,ft, &ao);
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
   FFJSON fa(FFJSON::ARRAY);
   fa[0]=1;
   FFJSON f("file://saveFileSample.ffjson|OBJECT");
   f["test"]="OK";
   //f["obj4"]["nestedFile"]["test"]="OK";
   FFJSON pvh;
   pvh = &f["vh"]["obj6"];
   pvh["users"]["test"]="OK";
   f["txoTest"]["test"]="OK";
   f["txoTest"].clearEFlag(FFJSON::FILE);
   cout << f << endl;
   f.save();
   // FFJSON ff("file://saveFileSample.ffjson");
   // FFJSON& ln = ff["vh"]["obj5"]["things"][].
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
   FFJSON f("file:///home/Necktwi/workspace/ferryfair/config.ffjson");
   cout << f << endl;
}

void test15 () {
   FFJSON f("file:///home/Necktwi/workspace/ferryfair/config.ffjson");
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
   FFJSON f("{1, 2, 3, 3}");
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

FFJSON t;
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

void test21 () {
   cout << t["a1"][666]["c8"] << endl;
   FFJSON t2;
   (int)t2["id"]==1;
   cout << t2 << endl;
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

void test23 () {
   cout << "===================================================" << endl;
   cout << "                       char[] test				      " << endl;
   cout << "===================================================" << endl;
	char un[48]= "gowtham";
	FFJSON fun;
	fun= un;
	cout << fun << endl;
}

int main (int argc, char** argv) {
   cout << "%SUITE_STARTING% TestFFJSON" << endl;
   cout << "%SUITE_STARTED%" << endl;
   
   FerryTimeStamp ftsStart;
   FerryTimeStamp ftsEnd;
   FerryTimeStamp ftsDiff;
   FerryTimeStamp ftsSuiteStart;
   FerryTimeStamp ftsSuiteEnd;
   ftsSuiteStart.update();

/*
   cout << "%TEST_STARTED% test1 (TestFFJSON)" << endl;
   ftsStart.update();
   test1();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff <<
      " test1 (TestFFJSON)" << endl;

   cout << "%TEST_STARTED% test2 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test2();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test2 (TestFFJSON)" << endl;
   
   cout << "%TEST_STARTED% test3 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test3();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test3 " << endl;

   cout << "%TEST_STARTED% test4 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test4();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test4 " << endl;

   cout << "%TEST_STARTED% test5 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test5();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test5 " << endl;
   
   cout << "%TEST_STARTED% test6 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test6();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test6 " << endl;
   
   cout << "%TEST_STARTED% test7 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test7();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test7 " << endl;
   
   cout << "%TEST_STARTED% test8 (TestFFJSON)\n" << endl;
   ftsStart.update();
   test8();
   ftsEnd.update();
   ftsDiff = ftsEnd-ftsStart;
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
   cout << "%TEST_STARTED% test23" << endl;
   ftsStart.update();
   test23();
   ftsEnd.update();
   ftsDiff = ftsEnd - ftsStart;
   cout << "%TEST_FINISHED% time=" << ftsDiff << " test23" << endl;

	ftsSuiteEnd.update();
   ftsDiff = ftsSuiteEnd-ftsSuiteStart;
   cout << "%SUITE_FINISHED% time=" << ftsDiff << endl;

   return (EXIT_SUCCESS);
}

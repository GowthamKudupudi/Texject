#include<iostream>
#include<map>
#include<vector>
#include<ferrybase/mystdlib.h>
#include<FFJSON.h>
using namespace std;

int child_exit_status=0;
FF_LOG_TYPE fflAllowedType = (FF_LOG_TYPE)(FFL_DEBUG | FFL_INFO);
unsigned int fflAllowedBlks = 9;

struct MyStruct{
    template<typename T>
    operator T&() {
        cout<<"size: "<<sizeof(T)<<endl;
        if (m_iSize == sizeof(T)) {
            return *reinterpret_cast<T*> (m_pV);
        }
        //return *reinterpret_cast<T*> (nullptr);
        return *reinterpret_cast<T*> (m_pV);
    }
    MyStruct(void* pV,int iSize){m_pV=pV;m_iSize=iSize;}
    void* m_pV=nullptr;
    int m_iSize=0;
};

int main (int argc, char** argv) {
    map<string, int> mStrInt;
    vector<map<string, int>::iterator > vStrIntIt;
    mStrInt["gowtham"]=100;
    mStrInt["satya"]=200;
    map<string,int>::iterator itmStrInt= mStrInt.begin();
    vStrIntIt.push_back(++itmStrInt);
    vStrIntIt.push_back(mStrInt.begin());
    vector<map<string,int>::iterator>::iterator ititmStrInt=vStrIntIt.begin();
    ititmStrInt++;
    cout<<(*ititmStrInt)->first<<endl;
    MyStruct mystruct((void*)new Txj(),sizeof(Txj));
    Txj& i=(Txj&)mystruct;
    cout<<i<<endl;
}

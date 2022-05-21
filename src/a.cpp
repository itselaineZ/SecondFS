#include <iostream>
#include <map>
using namespace std;
class Buf{
    int aaa;
    char* b;
}kk;
int main(){
    map <int, Buf*> mp;
    mp[5] = &kk;
    cout<<mp.size()<<endl;
    return 0;
}
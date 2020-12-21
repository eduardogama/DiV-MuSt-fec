#ifndef SERVER_TABLE_LIST_HH_
#define SERVER_TABLE_LIST_HH_

#include <map>
#include <string>

using namespace std;

class ServerTableSingleton
{
  public:
  static ServerTableSingleton* getInstance() {
    if (instance == 0)
      instance = new ServerTableSingleton();
    return instance;
  }

  string getServer(string ipvsrc) {
    return table_list[ipvsrc];
  }

  string addServer(string ipvsrc, string server) {
    return table_list[ipvsrc] = server;
  }

  private:
    static ServerTableSingleton* instance;
    ServerTableSingleton(){}

    map<string, string> table_list;

};

#endif // SERVER_TABLE_LIST_HH_

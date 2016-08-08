#include <cstdlib>
#include <iostream>
#include <exception>
#include <string>
#include <sstream>

#include "main.hpp"

int main(int argc, char** argv) {
  try {
    app::client client("http://127.0.0.1:9001/RPC2");
    std::string cmd;
    
    while(true) {
      cmd.clear();
      system("clear");
      std::cout << "[ supervisor dynamic consumers ]" << std::endl;
      std::cout << "--------------------------------" << std::endl;
      
      std::cout << client.response();
      
      std::cout << "> ";
      std::getline(std::cin, cmd);
      
      if(cmd == "q" || cmd == "quit" || cmd == "exit")
        break;
      else
        client.execute(cmd);
    }
    
    std::cout << "  Exiting..." << std::endl;
    
  } catch(std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  } catch(...) {
    std::cerr << "Unexpected exception." << std::endl;
  }
  
  return 0;
}

#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>

#include <vector>
#include <map>
#include <utility>
#include <cstdlib>

namespace app { 
  class client;

  class exception : public std::exception {
    public:
    std::string _message;
    exception(std::string const& message) : std::exception(), _message(message) {
      
    }
    
    const char* what() const noexcept {
      return _message.c_str();
    }
  };
  
  class method {
    friend class client;
    
    private:
    std::string                          _name;
    std::vector<xmlrpc_c::value::type_t> _types;
    xmlrpc_c::paramList                  _params;
    size_t                               _cursor;
    
    public:
    method(std::string const& name) : _name(name), _cursor(0) {
      
    }
    
    method(app::method const& that) {
      _name   = that._name;
      _types  = that._types;
      _params = that._params;
      _cursor = that._cursor;
    }
    
    app::method& operator = (app::method const& that) {
      if(this != &that) {
        _name   = that._name;
        _types  = that._types;
        _cursor = that._cursor;
      }
      return *this;
    }
    
    public:
    void type(xmlrpc_c::value::type_t type) {
      _types.push_back(type);
    }
    
    app::method& param(std::string const& param) {
      if(_cursor == 0)
        _params = xmlrpc_c::paramList(0);
      xmlrpc_c::value::type_t& type = _types[_cursor];
      switch(type) {
        case xmlrpc_c::value::type_t::TYPE_I8:
        case xmlrpc_c::value::type_t::TYPE_INT: {
          _params.addc(atoi(param.c_str()));
        } break;
        case xmlrpc_c::value::type_t::TYPE_BOOLEAN: {
          if(param == "false")
            _params.addc(false);
          else
            _params.addc(true);
        } break;
        case xmlrpc_c::value::type_t::TYPE_DOUBLE: {
          _params.addc(atof(param.c_str()));
        } break;
        case xmlrpc_c::value::type_t::TYPE_STRING: {
          _params.addc(param);
        } break;
        case xmlrpc_c::value::type_t::TYPE_ARRAY: {
          throw app::exception(std::string("Array not implemented!"));
        } break;
        case xmlrpc_c::value::type_t::TYPE_NIL: {
          throw app::exception(std::string("Nil not implemented!"));
        } break;
        default: {
          throw app::exception(std::string("Data type not supported!"));
        } break;
      }
      ++_cursor;
      _cursor = _cursor == _types.size() ? 0 : _cursor;
      return *this;
    }
    
    void name(std::string const& name) {
      _name = name;
    }
    
    std::string name() const {
      return _name;
    }
    
    xmlrpc_c::paramList& params() {
      return _params;
    }
  };
  
  class response {
    friend class client;
    
    private:
    xmlrpc_c::value _value;
    app::method*    _method;
    
    public:
    response() : _method(nullptr) {
      
    }
    
    response(xmlrpc_c::value const& value) : _value(value), _method(nullptr) {
      
    }
    
    response(app::response const& that) {
      _value  = that._value;
      _method = that._method;
    }
    
    response& operator = (app::response const& that) {
      if(this != &that) {
        _value  = that._value;
        _method = that._method;
      }
      return *this;
    }
    
    operator xmlrpc_c::value () {
      return _value;
    }
    
    operator xmlrpc_c::value () const {
      return _value;
    }
    
    public:
    xmlrpc_c::value& value() {
      return _value;
    }
    
    app::method* method() {
      return _method;
    }
    
    friend std::ostream& operator << (std::ostream&, app::response&);
  };

  class client {
    private:
    std::string                         _url;
    xmlrpc_c::clientSimple*             _client;
    app::response*                      _response;
    std::map<std::string, app::method*> _methods;
    
    public:
    client(std::string const& url) : _url(url), _client(new xmlrpc_c::clientSimple), _response(new app::response) {
      //call(app::method("system.listMethods"));
      
      app::method* signature = new method("system.methodSignature");
      signature->type(xmlrpc_c::value::type_t::TYPE_STRING);
      _methods.insert(std::make_pair("system.methodSignature", signature));
      _methods.insert(std::make_pair("system.listMethods", new method("system.listMethods")));
      _methods.insert(std::make_pair("clear", new method("clear")));
      
      //xmlrpc_c::carray data = xmlrpc_c::value_array(*_response).vectorValueValue();
      //for(xmlrpc_c::value value : data)
      //  _methods.insert(std::make_pair(xmlrpc_c::value_string(value), nullptr));
    }
    
    virtual ~client() { 
      for(auto it = _methods.begin(); it != _methods.end(); ++it)
        delete it->second;
      delete _response;
      delete _client;
    }
    
    private:
    void call(app::method* method) {
      if(method != nullptr) {
        delete _response;
        _response = new app::response;
        _response->_method = method;
        if(method->_params.size()) {
          _client->call(_url, method->_name, method->_params, &(_response->_value));
        } else {
          if(method->name() == "clear")
            return;
          else
            _client->call(_url, method->_name, &(_response->_value));
        }
      } else {
        throw app::exception(std::string("Method could not be created!"));
      }
    }
    
    void parse(std::string const& name, app::method* method) {
      std::cout << "client::parse('"<< name <<"', app::method*)" << std::endl;
      char param[64];
      char params[256];
      int n = sscanf(name.c_str(), "%s %s", param, params);
      if(n != 0)
        method->param(std::string(param));
      if(n == 2)
        parse(params, method);
    }
    
    public:
    client& execute(std::string& command) throw(app::exception) {
      // system.listMethods
      // system.methodSignature <string> // name
      // system.methodHelp <string> // name
      // supervisor.getProcessInfo <string> // name
      // supervisor.getAllProcessInfo
      // supervisor.getState
      // supervisor.readProcessStdoutLog <string> <int> <int> // name, offset, length
      
      if(command == "?")
        command = "system.listMethods";
      
      app::method* method = nullptr;
      {
        char name[64];
        char params[256];
        int n = sscanf(command.c_str(), "%s %s", name, params);
        if(n != 0) {
          auto it = _methods.find(name);
          if(it == _methods.end() || it->second == nullptr) {
            method = new app::method(name);
            
            {
              app::method* signature = _methods[std::string("system.methodSignature")];
              signature->param(name);
              
              call(signature);
              
              xmlrpc_c::carray data = xmlrpc_c::value_array(*_response).vectorValueValue();
              
              for(size_t i = 1; i < data.size(); ++i) { // skip 0 = return type for now
                xmlrpc_c::value& value = data[i];
                std::string svalue = xmlrpc_c::value_string(value); 
                if(svalue == "int") {
                  method->type(xmlrpc_c::value::type_t::TYPE_INT);
                } else if(svalue == "float") {
                  method->type(xmlrpc_c::value::type_t::TYPE_DOUBLE);
                } else if(svalue == "double") {
                  method->type(xmlrpc_c::value::type_t::TYPE_DOUBLE);
                } else if(svalue == "string") {
                  method->type(xmlrpc_c::value::type_t::TYPE_STRING);
                } else if(svalue == "boolean") {
                  method->type(xmlrpc_c::value::type_t::TYPE_BOOLEAN);
                } else if(svalue == "array") {
                  method->type(xmlrpc_c::value::type_t::TYPE_ARRAY);
                } else {
                  throw app::exception(std::string("Data type not supported!"));
                }
                
                std::cout << svalue << std::endl;
              }
              
              _methods.insert(std::make_pair(std::string(name), method));
            }
          } else {                          // found
            method = it->second;
          }
          
          if(n == 2) {
            parse(params, method);
          }
        } else {
          throw app::exception(std::string("No method found!"));
        }
      }
      
      call(method);
      
      return *this;
    }
    
    app::response& response() {
      return *_response;
    }
  
    std::map<std::string, app::method*>& methods() {
      return _methods;
    }
  };
  
  std::ostream& operator << (std::ostream& out, app::response& response) {
    static short spaces = 0;
    
    if(!response.value().isInstantiated())
      return out;
    
    if(spaces == 0 && response.method() != nullptr) {
      app::method* method = response.method();
      std::string str;
      
      str.append(response.method()->name());
      str.append("(");
      
      xmlrpc_c::paramList& params = method->params();
      for(size_t i = 0, l = params.size(); i < l; ++i) {
        str.append(params.getString(i));
        if(i < l - 1) 
          str.append(", ");
      }
      
      str.append(")");
      
      out << "> " << str << ": \n" << std::endl;
    }
    
    spaces++;
    
    switch(response.value().type()) {
      case xmlrpc_c::value::type_t::TYPE_INT: {
        out << std::string(spaces, ' ') << static_cast<int>(xmlrpc_c::value_int(response._value)) << std::endl;
      } break;
      case xmlrpc_c::value::type_t::TYPE_BOOLEAN: {
        out << std::string(spaces, ' ') << static_cast<bool>(xmlrpc_c::value_boolean(response._value)) << std::endl; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_DOUBLE:  {
        out << std::string(spaces, ' ') << static_cast<double>(xmlrpc_c::value_double(response._value)) << std::endl; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_DATETIME: {
        out << std::string(spaces, ' ') << xmlrpc_c::value_datetime(response._value).iso8601Value() << std::endl; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_STRING: {
        out << std::string(spaces, ' ') << static_cast<std::string>(xmlrpc_c::value_string(response._value)) << std::endl; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_BYTESTRING: {
        xmlrpc_c::cbytestring data = xmlrpc_c::value_bytestring(response._value).vectorUcharValue();
        out << std::string(spaces, ' ');
        for(unsigned char ch : data)
          out << ch;
        out << std::endl;
      } break;
      case xmlrpc_c::value::type_t::TYPE_ARRAY:  {
        xmlrpc_c::carray data = xmlrpc_c::value_array(response._value).vectorValueValue();
        for(xmlrpc_c::value& value : data) {
          app::response response(value);
          out << response;
        } 
      } break;
      case xmlrpc_c::value::type_t::TYPE_STRUCT: {
        xmlrpc_c::cstruct data = xmlrpc_c::value_struct(response._value);
        for(auto it = data.begin(); it != data.end(); ++it) {
          out << std::string(spaces+1, ' ') << it->first << ":";
          app::response response(it->second);
          out << response;
        }
      } break;
      case xmlrpc_c::value::type_t::TYPE_C_PTR: {
        out << "TYPE_C_PTR\n"; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_NIL: {
        out << "TYPE_NIL\n"; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_I8: {
        out << "TYPE_I8\n"; 
      } break;
      case xmlrpc_c::value::type_t::TYPE_DEAD: {
        out << "TYPE_DEAD\n"; 
      } break;
      default: {
        out << "ERROR!\n"; 
      } break;
    }
    spaces--;
    
    if(spaces == 0)
      out << std::endl;
    
    return out;
  }
}


















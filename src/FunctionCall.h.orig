#pragma once

class FunctionCall {
public:
  string name;
  int id;
  int parent;
  int scriptId;
  int parentScriptId;
  bool withinScript = true;
  string function_id;
  uint64_t ts;
  
  FunctionCall() {}

  bool operator==(const int id) {
    return this->id == id;
  }
};
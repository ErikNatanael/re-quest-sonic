#pragma once

class FunctionCall {
public:
  string name;
  int id;
  int parent;
  int scriptId;
  uint64_t ts;
  
  FunctionCall() {}
};
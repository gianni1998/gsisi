#include "IROptimiser.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void expect(bool c, const char* m) { if(!c) throw std::runtime_error(m); }
static IRInstruction make(IROp op) { IRInstruction i{}; i.op=op; return i; }
IRInstruction ci(std::string d,int v){ auto i=make(IROp::ConstInt); i.type=Type::Int;i.dst=std::move(d);i.intValue=v;return i;}
IRInstruction cb(std::string d,bool v){ auto i=make(IROp::ConstBool); i.type=Type::Bool;i.dst=std::move(d);i.boolValue=v;return i;}
IRInstruction bin(IROp op,std::string d,std::string a,std::string b,Type t){auto i=make(op);i.type=t;i.dst=std::move(d);i.args={std::move(a),std::move(b)};return i;}
IRInstruction ret(std::string a=""){auto i=make(IROp::Return); if(!a.empty())i.args={std::move(a)};return i;}
IRInstruction jump(std::string t){auto i=make(IROp::Jump);i.target=std::move(t);return i;}
IRInstruction branch(std::string c,std::string t,std::string f){auto i=make(IROp::Branch);i.args={std::move(c)};i.trueTarget=std::move(t);i.falseTarget=std::move(f);return i;}
IRInstruction call(std::string c,std::vector<std::string> a={}){auto i=make(IROp::Call);i.callee=std::move(c);i.args=std::move(a);return i;}

IRProgram programOf(std::vector<IRBlock> blocks) {
    IRFunction f; f.name="main"; f.returnType=Type::Int; f.blocks=std::move(blocks);
    IRProgram p; p.functions.push_back(std::move(f)); return p;
}
IRBlock block(std::string n,std::vector<IRInstruction> is){return IRBlock{std::move(n),std::move(is)};}

int main(){
 int passed=0;
 auto run=[&](const char* n, auto fn){try{fn();++passed;std::cout<<"PASS "<<n<<"\n";}catch(const std::exception&e){std::cerr<<"FAIL "<<n<<": "<<e.what()<<"\n";std::exit(1);}};

 run("fold arithmetic", []{
   auto p=programOf({block("entry",{ci("%t0",5),ci("%t1",5),bin(IROp::Add,"%t2","%t0","%t1",Type::Int),ret("%t2")})});
   IROptimiser{}.optimise(p);
   auto &v=p.functions[0].blocks[0].instructions;
   expect(v.size()==2,"expected const + return");
   expect(v[0].op==IROp::ConstInt && v[0].intValue==10 && v[0].dst=="%t2","fold bad");
 });
 run("fold comparison branch and unreachable", []{
   auto p=programOf({
      block("entry",{ci("%a",3),ci("%b",2),bin(IROp::Greater,"%c","%a","%b",Type::Bool),branch("%c","then","else")}),
      block("then",{ci("%x",1),ret("%x")}),
      block("else",{ci("%y",0),ret("%y")})
   });
   IROptimiser{}.optimise(p);
   expect(p.functions[0].blocks.size()==2,"else should be unreachable");
   auto &e=p.functions[0].blocks[0].instructions;
   expect(e.size()==1 && e[0].op==IROp::Jump && e[0].target=="then","branch not simplified");
 });
 run("remove unreachable side effects", []{
   auto p=programOf({
      block("entry",{cb("%c",false),branch("%c","body","end")}),
      block("body",{ci("%x",999),call("out",{"%x"}),jump("end")}),
      block("end",{ci("%z",0),ret("%z")})
   });
   IROptimiser{}.optimise(p);
   expect(p.functions[0].blocks.size()==2,"body should be removed");
   for(auto &b:p.functions[0].blocks) expect(b.name!="body","body remains");
 });
 run("iterative dce", []{
   auto p=programOf({block("entry",{ci("%a",1),ci("%b",2),bin(IROp::Add,"%dead","%a","%b",Type::Int),ci("%r",0),ret("%r")})});
   IROptimiser{}.optimise(p);
   auto &v=p.functions[0].blocks[0].instructions;
   expect(v.size()==2,"dead chain remains");
   expect(v[0].dst=="%r","wrong survivor");
 });
 run("preserve divide by zero", []{
   auto p=programOf({block("entry",{ci("%a",1),ci("%b",0),bin(IROp::Divide,"%q","%a","%b",Type::Int),ci("%r",0),ret("%r")})});
   IROptimiser{}.optimise(p);
   bool found=false; for(auto&i:p.functions[0].blocks[0].instructions) if(i.op==IROp::Divide) found=true;
   expect(found,"trapping divide removed/folded");
 });
 run("preserve calls", []{
   auto p=programOf({block("entry",{ci("%x",42),call("out",{"%x"}),ci("%r",0),ret("%r")})});
   IROptimiser{}.optimise(p);
   bool found=false; for(auto&i:p.functions[0].blocks[0].instructions) if(i.op==IROp::Call) found=true;
   expect(found,"call removed");
 });
 run("fold bool not", []{
   auto n=make(IROp::Not); n.type=Type::Bool;n.dst="%n";n.args={"%b"};
   auto p=programOf({
     block("entry",{cb("%b",false),n,branch("%n","yes","no")}),
     block("yes",{ci("%r",0),ret("%r")}),
     block("no",{ci("%q",1),ret("%q")})
   });
   IROptimiser{}.optimise(p);
   expect(p.functions[0].blocks.size()==2,"wrong bool fold");
   expect(p.functions[0].blocks[1].name=="yes","not false should choose yes");
 });
 std::cout<<passed<<"/7 tests passed\n";
}
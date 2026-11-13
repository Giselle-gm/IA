#include "greedy_variants.h"
#include "parser.h"
#include "evaluator.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
namespace fs=std::filesystem;
using Clock=chrono::steady_clock;
struct Summary { string name; int runs=0,feasible=0; double seconds=0.0,objective=0.0; };
string csv(const string& value) {
    string out="\"";
    for(char c:value) { if(c=='"') out+='"'; out+=c; }
    return out+"\"";
}
int main() {
    const fs::path root="instances";
    if(!fs::exists(root)||!fs::is_directory(root)) { cerr<<"No se encontro instances/.\n"; return 1; }
    vector<fs::path> files;
    for(const auto& entry:fs::recursive_directory_iterator(root))
        if(entry.is_regular_file()&&entry.path().extension()==".txt") files.push_back(entry.path());
    sort(files.begin(),files.end());
    vector<Summary> summaries={{"Greedy con backtracking"},{"Greedy sin backtracking"}};
    ofstream output("greedy_backtracking_comparison.csv");
    if(!output) { cerr<<"No se pudo crear greedy_backtracking_comparison.csv.\n"; return 2; }
    output<<"instancia,algoritmo,factible,clientes_asignados,nodos_explorados,retrocesos,tiempo_segundos,objetivo,vehiculos,costo_viaje\n";
    int read=0,errors=0,both=0,onlyBt=0,onlyNoBt=0,neither=0;
    for(const fs::path& file:files) {
        Instance instance;
        if(!readInstance(file.string(),instance)) { ++errors; continue; }
        ++read;
        GreedyVariantResult results[2];
        auto run=[&](int index,auto algorithm) {
            const auto start=Clock::now();
            results[index]=algorithm(instance);
            const double seconds=chrono::duration<double>(Clock::now()-start).count();
            Summary& summary=summaries[index]; ++summary.runs; summary.seconds+=seconds;
            if(results[index].success) { ++summary.feasible; summary.objective+=results[index].solution.objectiveValue; }
            output<<csv(file.generic_string())<<","<<csv(summary.name)<<","
                  <<(results[index].success?1:0)<<","<<results[index].assignedCustomers<<","
                  <<results[index].exploredNodes<<","<<results[index].backtracks<<","
                  <<fixed<<setprecision(6)<<seconds<<",";
            if(results[index].success) {
                const Evaluation e=evaluateSolution(instance,results[index].solution);
                output<<fixed<<setprecision(2)<<e.objectiveValue<<","<<e.vehiclesUsed<<","<<e.travelCost<<"\n";
            } else output<<",,,\n";
        };
        run(0,buildGreedyBacktracking); run(1,buildGreedyNoBacktracking);
        const bool a=results[0].success,b=results[1].success;
        if(a&&b) ++both; else if(a) ++onlyBt; else if(b) ++onlyNoBt; else ++neither;
    }
    cout<<fixed<<setprecision(3)<<"\n========== COMPARACION GREEDY CON Y SIN BACKTRACKING ==========\n"
        <<"Instancias encontradas: "<<files.size()<<"\nInstancias leidas: "<<read
        <<"\nErrores de lectura: "<<errors<<"\n\n";
    for(const Summary& s:summaries) {
        cout<<"--- "<<s.name<<" ---\nSoluciones factibles: "<<s.feasible<<"/"<<s.runs
            <<"\nTiempo total: "<<s.seconds<<" s\nTiempo promedio: "
            <<(s.runs?s.seconds/s.runs:0.0)<<" s\nObjetivo promedio (solo factibles): "
            <<(s.feasible?s.objective/s.feasible:0.0)<<"\n\n";
    }
    cout<<"--- Comparacion por instancia ---\nResueltas por ambas: "<<both
        <<"\nSolo backtracking: "<<onlyBt<<"\nSolo sin backtracking: "<<onlyNoBt
        <<"\nNinguna: "<<neither<<"\n\nDetalle: greedy_backtracking_comparison.csv\n";
    return errors==0?0:3;
}

#include "greedy_variants.h"
#include "evaluador.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace {
constexpr double FIXED_VEHICLE_COST=100000.0;
constexpr int MAX_SEARCH_NODES=20000;
constexpr int MAX_BACKTRACKS=5000;

struct Candidate {
    Solucion solucion;
    double objective=0.0;
    int priority=0, customerId=-1, optionId=-1, routeIndex=-1, position=-1;
};

//Une paradas consecutivas en una ruta que correspondan a la misma ubicación.
void mergeConsecutiveStops(Route& route) {
    vector<Stop> merged;
    
    for(const Stop& stop:route.stops) {
        if(!merged.empty() && merged.back().locationId==stop.locationId) {
            auto& ids=merged.back().optionIds;
            ids.insert(ids.end(),stop.optionIds.begin(),stop.optionIds.end());
        } else merged.push_back(stop);
    }
    route.stops=std::move(merged);
}

//Costo de viaje de una ruta
double routeCost(const Instance& instance,const Route& route) {
    if(route.stops.empty()) return 0.0;
    double cost=0.0; int previous=0;
    
    for(const Stop& stop:route.stops) {
        cost+=travelCost(instance.locations[previous],instance.locations[stop.locationId]);
        previous=stop.locationId;
    }
    return cost+travelCost(instance.locations[previous],instance.locations[0]);
}

double partialObjective(const Instance& instance,const Solucion& solucion) {
    double value=FIXED_VEHICLE_COST*solucion.routes.size();
    
    for(const Route& route:solucion.routes) value+=routeCost(instance,route);
    return value;
}

bool serviceLevelsCanStillBeMet(const Instance& instance,const Solucion& solucion) {
    const int n=static_cast<int>(instance.customers.size());
    
    for(int level=0;level<static_cast<int>(instance.minimumServiceLevel.size());++level) {
        int satisfied=0,potential=0;
        
        for(int customer=0;customer<n;++customer) {
            const int selected=solucion.selectedOptions[customer];
            if(selected>=0) {
                if(instance.options[selected].priority<=level) ++satisfied;
                continue;
            }
            bool possible=false;
            
            for(int optionId:instance.optionsByCustomer[customer]) {
                if(optionId>=0 && optionId<static_cast<int>(instance.options.size()) &&
                   instance.options[optionId].priority<=level) { possible=true; break; }
            }
            if(possible) ++potential;
        }
        const int required=static_cast<int>(ceil(instance.minimumServiceLevel[level]*n));
        if(satisfied+potential<required) return false;
    }
    return true;
}

bool partialSolucionFactible(const Instance& instance,const Solucion& solucion) {
    const int customers=static_cast<int>(instance.customers.size());
    const int locations=static_cast<int>(instance.locations.size());
    const int options=static_cast<int>(instance.options.size());
    
    if(static_cast<int>(solucion.routes.size())>instance.vehicleCount ||
       static_cast<int>(solucion.selectedOptions.size())!=customers) return false;
    vector<int> sdlLoads(locations,0);
    
    for(int customer=0;customer<customers;++customer) {
        const int optionId=solucion.selectedOptions[customer];
        if(optionId<0) continue;
        if(optionId>=options || instance.options[optionId].customerId!=customer) return false;
        const int locationId=instance.options[optionId].locationId;
        if(locationId<0 || locationId>=locations) return false;
        if(instance.locations[locationId].type==1) ++sdlLoads[locationId];
    }
    
    for(int id=0;id<locations;++id) {
        const Location& location=instance.locations[id];
        if(location.type==1 && location.capacity>=0 && sdlLoads[id]>location.capacity) return false;
    }
    
    for(const Route& route:solucion.routes) {
        if(route.stops.empty()) return false;
        int load=0,currentTime=0,previousLocation=0;
        set<int> routeCustomers;
        
        for(const Stop& stop:route.stops) {
            const int locationId=stop.locationId;
            
            if(locationId<=0 || locationId>=locations || stop.optionIds.empty()) return false;
            const Location& location=instance.locations[locationId];
            currentTime+=travelTime(instance.locations[previousLocation],location);
            const int serviceStart=max(currentTime,location.ready);
            if(serviceStart>location.due) return false;
            int serviceDuration=location.serviceTime;
            
            for(int optionId:stop.optionIds) {
                if(optionId<0 || optionId>=options) return false;
                const Option& option=instance.options[optionId];
                const int customerId=option.customerId;
                if(customerId<0 || customerId>=customers || option.locationId!=locationId ||
                   solucion.selectedOptions[customerId]!=optionId ||
                   !routeCustomers.insert(customerId).second) return false;
                load+=instance.customers[customerId].demand;
                serviceDuration+=option.serviceTime;
            }
            if(load>instance.vehicleCapacity) return false;
            currentTime=serviceStart+serviceDuration;
            previousLocation=locationId;
        }
    }
    return serviceLevelsCanStillBeMet(instance,solucion);
}

bool candidateLess(const Candidate& a,const Candidate& b) {
    if(a.objective!=b.objective) return a.objective<b.objective;
    if(a.priority!=b.priority) return a.priority<b.priority;
    if(a.customerId!=b.customerId) return a.customerId<b.customerId;
    if(a.optionId!=b.optionId) return a.optionId<b.optionId;
    if(a.routeIndex!=b.routeIndex) return a.routeIndex<b.routeIndex;
    return a.position<b.position;
}

vector<Candidate> generateCandidates(const Instance& instance,const Solucion& partial,int customerId) {
    vector<Candidate> candidates;
    
    for(int optionId:instance.optionsByCustomer[customerId]) {
        
        if(optionId<0 || optionId>=static_cast<int>(instance.options.size())) continue;
        const Option& option=instance.options[optionId];
        
        if(option.customerId!=customerId) continue;
        
        for(int routeIndex=0;routeIndex<static_cast<int>(partial.routes.size());++routeIndex) {
            const Route& route=partial.routes[routeIndex];
            
            for(int position=0;position<=static_cast<int>(route.stops.size());++position) {
                Candidate candidate;
                
                candidate.solucion=partial;
                candidate.customerId=customerId; candidate.optionId=optionId;
                candidate.routeIndex=routeIndex; candidate.position=position;
                candidate.priority=option.priority;
                candidate.solucion.selectedOptions[customerId]=optionId;
                Stop stop; stop.locationId=option.locationId; stop.optionIds.push_back(optionId);
                auto& stops=candidate.solucion.routes[routeIndex].stops;
                stops.insert(stops.begin()+position,stop);
                mergeConsecutiveStops(candidate.solucion.routes[routeIndex]);
                
                if(!partialSolucionFactible(instance,candidate.solucion)) continue;
                candidate.objective=partialObjective(instance,candidate.solucion);
                candidates.push_back(std::move(candidate));
            }
        }
        if(static_cast<int>(partial.routes.size())<instance.vehicleCount) {
            Candidate candidate;
            
            candidate.solucion=partial;
            candidate.customerId=customerId; candidate.optionId=optionId;
            candidate.routeIndex=static_cast<int>(partial.routes.size());
            candidate.priority=option.priority;
            candidate.solucion.selectedOptions[customerId]=optionId;
            Route route; route.vehicleId=candidate.routeIndex;
            Stop stop; stop.locationId=option.locationId; stop.optionIds.push_back(optionId);
            route.stops.push_back(stop); candidate.solucion.routes.push_back(route);
            
            if(partialSolucionFactible(instance,candidate.solucion)) {
                candidate.objective=partialObjective(instance,candidate.solucion);
                candidates.push_back(std::move(candidate));
            }
        }
    }
    sort(candidates.begin(),candidates.end(),candidateLess);
    return candidates;
}

vector<int> customerOrder(const Instance& instance) {
    vector<int> order(instance.customers.size());
    
    for(int i=0;i<static_cast<int>(order.size());++i) order[i]=i;
    stable_sort(order.begin(),order.end(),[&](int a,int b) {
        const size_t oa=instance.optionsByCustomer[a].size(),ob=instance.optionsByCustomer[b].size();
        if(oa!=ob) return oa<ob;
        
        if(instance.customers[a].demand!=instance.customers[b].demand)
            return instance.customers[a].demand>instance.customers[b].demand;
        return a<b;
    });
    return order;
}

bool searchWithBacktracking(const Instance& instance,const vector<int>& order,int position,
                            Solucion& solucion,GreedyVariantResult& stats) {
    if(position==static_cast<int>(order.size())) return true;
    
    const int customerId=order[position];
    const vector<Candidate> candidates=generateCandidates(instance,solucion,customerId);
    
    if(candidates.empty()) return false;
    
    for(const Candidate& candidate:candidates) {
    
        if(stats.exploredNodes>=MAX_SEARCH_NODES || stats.backtracks>=MAX_BACKTRACKS) return false;
        ++stats.exploredNodes;
        
        Solucion next=candidate.solucion;
        
        if(searchWithBacktracking(instance,order,position+1,next,stats)) {
            solucion=std::move(next); return true;
        }
        ++stats.backtracks;
        if(stats.exploredNodes>=MAX_SEARCH_NODES || stats.backtracks>=MAX_BACKTRACKS) return false;
    }
    return false;
}

GreedyVariantResult finish(const Instance& instance,Solucion solucion,
                           GreedyVariantResult result,const string& name) {
    result.assignedCustomers=0;
    for(int id:solucion.selectedOptions) if(id>=0) ++result.assignedCustomers;
    
    const Evaluacion evaluacion=evaluarSolucion(instance,solucion);
    
    if(!evaluacion.factible) {
        result.message=name+": la solucion no supero la validacion final.";
        if(!evaluacion.violations.empty()) result.message+=" "+evaluacion.violations.front();
        return result;
    }
    solucion.objectiveValue=evaluacion.objectiveValue;
    result.solucion=std::move(solucion); result.success=true;
    result.message=name+": solucion factible.";

    return result;
}

bool validInstance(const Instance& instance) {
    return !instance.customers.empty() && !instance.locations.empty() &&
        instance.vehicleCount>0 && instance.vehicleCapacity>0 &&
        instance.optionsByCustomer.size()==instance.customers.size();
}
}

GreedyVariantResult buildGreedyBacktracking(const Instance& instance) {
    GreedyVariantResult result;
    
    if(!validInstance(instance)) { result.message="Instancia invalida o incompleta."; return result; }
    
    Solucion solucion; solucion.selectedOptions.assign(instance.customers.size(),-1);
    
    if(!searchWithBacktracking(instance,customerOrder(instance),0,solucion,result)) {
        result.message="Greedy con backtracking limitado no encontro solucion. Nodos: "+
            to_string(result.exploredNodes)+", retrocesos: "+to_string(result.backtracks)+".";
        for(int id:solucion.selectedOptions) if(id>=0) ++result.assignedCustomers;
        return result;
    }
    return finish(instance,std::move(solucion),std::move(result),"Greedy con backtracking limitado");
}

GreedyVariantResult buildGreedyNoBacktracking(const Instance& instance) {
    GreedyVariantResult result;
    
    if(!validInstance(instance)) { result.message="Instancia invalida o incompleta."; return result; }
    
    Solucion solucion; solucion.selectedOptions.assign(instance.customers.size(),-1);
    
    for(int customerId:customerOrder(instance)) {
        const vector<Candidate> candidates=generateCandidates(instance,solucion,customerId);
        if(candidates.empty()) {
            result.message="Greedy sin backtracking se atasco en el cliente "+to_string(customerId)+".";
            for(int id:solucion.selectedOptions) if(id>=0) ++result.assignedCustomers;
            return result;
        }
        ++result.exploredNodes;
        solucion=candidates.front().solucion;
    }
    return finish(instance,std::move(solucion),std::move(result),"Greedy sin backtracking");
}

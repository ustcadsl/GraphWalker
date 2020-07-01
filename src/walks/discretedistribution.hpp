#ifndef DEF_DISCRETE_DISTRIBUTION
#define DEF_DISCRETE_DISTRIBUTION

#include <queue>
#include "api/datatype.hpp"

const unsigned capacity = 1000;

struct IdCount {
    vid_t id;
    uint16_t count;

    IdCount(vid_t id, uint16_t count) {
        this->id = id;
        this->count = count;
    }

    bool operator<(const IdCount ic) const {
        return (count > ic.count);
    }
};

class DiscreteDistribution{

public:
	vid_t* ids;
	uint16_t* counts;
	unsigned size;

public:
	DiscreteDistribution(){
		size = 0;
        ids = (vid_t*)malloc(capacity*sizeof(vid_t));
		counts = (uint16_t*)malloc(capacity*sizeof(uint16_t));
        std::memset(counts, 0, capacity*sizeof(uint16_t));
	}

	~DiscreteDistribution(){
		free(ids);
		free(counts);
	}

    void add(vid_t _id){
        for(unsigned i = 0; i < size; i++){
            if(ids[i] == _id){
                counts[i]++;
                return ;
            }
        }
        #pragma omp critical
        {
            while(size >= capacity){
                filter(2);
            }
        }
        ids[size] = _id;
        counts[size] = 1;
        size++;
    }

    void filter(uint16_t mincount){
        int idx = 0;
        for(unsigned i=0; i < size; i++) {
            if (counts[i] >= mincount) {
                ids[idx] = ids[i];
                counts[idx] = (uint16_t) (counts[i] - mincount + 1);
                idx++;
            }
        }
        size = idx;
    }

    void getTop(unsigned ntop){
        logstream(LOG_INFO) << "getTop " << ntop << " from size = " << size << std::endl;
        std::priority_queue<IdCount> topDist;
        IdCount minIC(ids[0],counts[0]);
        for(unsigned i = 0; i < size; i++){
            if(i < ntop){
                topDist.push( IdCount(ids[i],counts[i]) );
                minIC = topDist.top();
            }else{
                if(minIC.count < counts[i]){
                    topDist.pop();
                    topDist.push(IdCount(ids[i],counts[i]));
                    minIC = topDist.top();
                }
            }
        }
        logstream(LOG_INFO) << "Top " << ntop << " visitfrequencies - " << std::endl;
        int i = 0;
        while(!topDist.empty()){
            IdCount ic = topDist.top();
            logstream(LOG_INFO) << i++ << "-\t" << ic.id << ":\t " << ic.count << std::endl;
            topDist.pop();
        }
    }
};

#endif
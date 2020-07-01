#ifndef RANDOMWALKWITHRESTART
#define RANDOMWALKWITHRESTART

#include <string>
#include <fstream>
#include <time.h>

#include "walks/walk.hpp" 
#include "api/datatype.hpp"

/**
 * Type definitions. Remember to create suitable graph shards using the
 * Sharder-program.
 */
 
class RandomWalkwithRestart : public RandomWalk {
public:
    vid_t source;

public:
    void initializeRW( vid_t _source, unsigned _nwalks, unsigned _nsteps, float _tail) {
        source = _source;
        nwalks = _nwalks;
        nsteps = _nsteps;
        tail = _tail;
        tailwalknum = (unsigned)(nwalks*tail);
        std::cout << "nwalks, nsteps, tail, tailwalknum : "  << nwalks << " " << nsteps << " " << tail << " " << tailwalknum << std::endl;
    }
    /**
     *  Walk update function.
     */
    //void updateByWalk(std::vector<graphchi_vertex<VertexDataType, EdgeDataType> > &vertices, vid_t vid, unsigned sub_interval_st, unsigned sub_interval_en, walkManager &walk_manager, graphchi_context &gcontext){
    void updateByWalk(WalkDataType walk, unsigned walkid, unsigned exec_interval, Vertex *&vertices, WalkManager &walk_manager ){ //, VertexDataType* vertex_value){
            unsigned threadid = omp_get_thread_num();
            WalkDataType nowWalk = walk;
            vid_t sourId = walk_manager.getSourceId(nowWalk);
            vid_t dstId = walk_manager.getCurrentId(nowWalk) + intervals[exec_interval].first;
            unsigned hop = walk_manager.getHop(nowWalk);
            unsigned seed = walkid+dstId+hop+(unsigned)time(NULL);
            while (dstId >= intervals[exec_interval].first && dstId <= intervals[exec_interval].second && hop < nsteps ){
                updateInfo(sourId, dstId, threadid, hop);
                Vertex &nowVertex = vertices[dstId - intervals[exec_interval].first];
                if (nowVertex.outd > 0 && ((float)rand_r(&seed))/RAND_MAX > 0.15 )
                    dstId = random_outneighbor(nowVertex, seed);
                else
                    dstId = walk_manager.getSourceId(walk);
                    // dstId = walk_manager.getSourceId(walk) + source;
                hop++;
                nowWalk++;
            }
            if( hop < nsteps ){
            // if( hop < nsteps && dstId != source){
                unsigned p = getInterval( dstId );
                walk_manager.moveWalk(nowWalk, p, threadid, dstId - intervals[p].first);
                walk_manager.setMinStep( p, hop );
            }
    }   

};

#endif
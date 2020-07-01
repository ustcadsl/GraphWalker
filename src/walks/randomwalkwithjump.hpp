#ifndef RANDOMWALKWITHJUMP
#define RANDOMWALKWITHJUMP

#include <string>
#include <fstream>
#include <time.h>
//#include <chrono>

#include "walks/walk.hpp" 
#include "api/datatype.hpp"

/**
 * Type definitions. Remember to create suitable graph shards using the
 * Sharder-program.
 */
 
class RandomWalkwithJump : public RandomWalk{

public:
        vid_t N;

public:

    void initializeRW(vid_t _N, wid_t _R, hid_t _L){
        N = _N;
        R = _R;
        L = _L;
    }

    void updateByWalk(WalkDataType walk, wid_t walkid, bid_t exec_block, eid_t *&beg_pos, vid_t *&csr, WalkManager &walk_manager ){ //, VertexDataType* vertex_value){
        tid_t threadid = omp_get_thread_num();
        WalkDataType nowWalk = walk;
        vid_t sourId = walk_manager.getSourceId(nowWalk);
        vid_t dstId = walk_manager.getCurrentId(nowWalk) + blocks[exec_block];
        hid_t hop = walk_manager.getHop(nowWalk);
        unsigned seed = (unsigned)(walkid+dstId+hop+(unsigned)time(NULL));
        while (dstId >= blocks[exec_block] && dstId < blocks[exec_block+1] && hop < L ){
            updateInfo(sourId, dstId, threadid, hop);
            vid_t dstIdp = dstId - blocks[exec_block];
            eid_t outd = beg_pos[dstIdp+1] - beg_pos[dstIdp];
            if (outd > 0 && (float)rand_r(&seed)/RAND_MAX > 0.15 ){
                eid_t pos = beg_pos[dstIdp] - beg_pos[0] + ((eid_t)rand_r(&seed))%outd;
                dstId = csr[pos];
            }else{
                dstId = rand_r(&seed) % N;
            }
            hop++;
            nowWalk++;
        }
        if( hop < L ){
            bid_t p = getblock( dstId );
            if(p>=nblocks) return;
            walk_manager.moveWalk(nowWalk, p, threadid, dstId - blocks[p]);
            walk_manager.setMinStep( p, hop );
            walk_manager.ismodified[p] = true;
        }
    }

};

#endif